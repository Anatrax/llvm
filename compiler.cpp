#include <iostream>
#include <map>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Value.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"

static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> TheBuilder(TheContext);
static llvm::Module* TheModule;

static std::map<std::string, llvm::Value*> TheSymbolTable;

llvm::Value* numericConstant(float val) {
  return llvm::ConstantFP::get(TheContext, llvm::APFloat(val));
}

llvm::Value* binaryOperation(llvm::Value* lhs, llvm::Value* rhs, char op) {
  if (!lhs || !rhs) {
    return NULL;
  }

  switch (op) {
    case '+':
      return TheBuilder.CreateFAdd(lhs, rhs, "addtmp");
    case '-':
      return TheBuilder.CreateFSub(lhs, rhs, "subtmp");
    case '*':
      return TheBuilder.CreateFMul(lhs, rhs, "multmp");
    case '/':
      return TheBuilder.CreateFDiv(lhs, rhs, "divtmp");
    case '<':
      lhs = TheBuilder.CreateFCmpULT(lhs, rhs, "lttmp");
      return TheBuilder.CreateUIToFP(
        lhs,
        llvm::Type::getFloatTy(TheContext),
        "booltmp"
      );
    default:
      std::cerr << "Invalid operator: " << op << std::endl;
      return NULL;
  }
}

llvm::AllocaInst* generateEntryBlockAlloca(const std::string& name) {
  llvm::Function* currFn = TheBuilder.GetInsertBlock()->getParent();
  llvm::IRBuilder<> tmpBuilder(
    &currFn->getEntryBlock(),
    currFn->getEntryBlock().begin()
  );
  return tmpBuilder.CreateAlloca(
    llvm::Type::getFloatTy(TheContext),
    0,
    name.c_str()
  );
}

llvm::Value* assignmentStatement(const std::string& lhs, llvm::Value* rhs) {
  if (!rhs) {
    return NULL;
  }

  /*
   * Have we already allocated space for lhs?
   */
  if (!TheSymbolTable.count(lhs)) {
    // Allocate space for lhs
    TheSymbolTable[lhs] = generateEntryBlockAlloca(lhs);
  }
  return TheBuilder.CreateStore(rhs, TheSymbolTable[lhs]);
}

llvm::Value* variableValue(const std::string& name) {
  llvm::Value* alloca = TheSymbolTable[name];
  if (!alloca) {
    std::cerr << "Unknown variable: " << name << std::endl;
    return NULL;
  }
  return TheBuilder.CreateLoad(alloca, name.c_str());
}

/*
 * if (b < 8) {
 *   c = a * b;
 * } else {
 *   c = a + b;
 * }
 */
llvm::Value* ifElseStatement() {
  /*
   * b < 8
   */
  llvm::Value* cond = binaryOperation(
    variableValue("b"),
    numericConstant(8),
    '<'
  );
  if (!cond) {
    return NULL;
  }
  cond = TheBuilder.CreateFCmpONE(cond, numericConstant(0), "ifcond");

  /*
   * Generate if block, else block, and continuation block.
   */
  llvm::Function* currFn = TheBuilder.GetInsertBlock()->getParent();
  llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(
    TheContext,
    "ifBlock",
    currFn
  );
  llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(
    TheContext,
    "elseBlock"
  );
  llvm::BasicBlock* continuationBlock = llvm::BasicBlock::Create(
    TheContext,
    "continuationBlock"
  );

  TheBuilder.CreateCondBr(cond, ifBlock, elseBlock);

  /*
   * Generate code in if block.
   */
  TheBuilder.SetInsertPoint(ifBlock);
  llvm::Value* aTimesB = binaryOperation(
    variableValue("a"),
    variableValue("b"),
    '*'
  );
  llvm::Value* ifBlockAssgn = assignmentStatement("c", aTimesB);
  TheBuilder.CreateBr(continuationBlock);

  /*
   * Generate code in else block.
   */
  currFn->getBasicBlockList().push_back(elseBlock);
  TheBuilder.SetInsertPoint(elseBlock);
  llvm::Value* aPlusB = binaryOperation(
    variableValue("a"),
    variableValue("b"),
    '+'
  );
  llvm::Value* elseBlockAssgn = assignmentStatement("c", aPlusB);
  TheBuilder.CreateBr(continuationBlock);

  /*
   * Insert following code into continuation block.
   */
  currFn->getBasicBlockList().push_back(continuationBlock);
  TheBuilder.SetInsertPoint(continuationBlock);
  return continuationBlock;
}

int generateMachineCode(const std::string& filename) {
  std::string targetTriple = llvm::sys::getDefaultTargetTriple();
  std::cerr << " Target triple: " << targetTriple << std::endl;

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  std::string error;
  const llvm::Target* target = llvm::TargetRegistry::lookupTarget(
    targetTriple,
    error
  );
  if (!target) {
    std::cerr << "Failed to look up target; error: " << error << std::endl;
    return 1;
  }

  llvm::TargetOptions options;
  llvm::TargetMachine* targetMachine = target->createTargetMachine(
    targetTriple,
    "generic",
    "",
    options,
    llvm::Optional<llvm::Reloc::Model>()
  );

  TheModule->setDataLayout(targetMachine->createDataLayout());
  TheModule->setTargetTriple(targetTriple);

  std::error_code ec;
  llvm::raw_fd_ostream outfile(filename, ec, llvm::sys::fs::F_None);
  if (ec) {
    std::cerr << "Could not open output file; error: " << ec.message() << std::endl;
    return 1;
  }

  llvm::legacy::PassManager pm;
  llvm::TargetMachine::CodeGenFileType fileType =
    llvm::TargetMachine::CGFT_ObjectFile;
  if (targetMachine->addPassesToEmitFile(pm, outfile, NULL, fileType)) {
    std::cerr << "Can't emit object file" << std::endl;
    return 1;
  }
  pm.run(*TheModule);
  outfile.close();
  return 0;
}

int main(int argc, char const *argv[]) {
  TheModule = new llvm::Module("LLVM_Demo", TheContext);

  /*
   * Set up function signature and target() function.
   */
  llvm::FunctionType* targetFnSignature = llvm::FunctionType::get(
    llvm::Type::getFloatTy(TheContext),
    false
  );
  llvm::Function* targetFn = llvm::Function::Create(
    targetFnSignature,
    llvm::GlobalValue::ExternalLinkage,
    "target",
    TheModule
  );

  /*
   * Set up entry block for target().
   */
  llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(
    TheContext,
    "entry",
    targetFn
  );
  TheBuilder.SetInsertPoint(entryBlock);

  /*
   * a = 8 + 4 * 2
   */
  llvm::Value* expr1 = binaryOperation(
    numericConstant(4),
    numericConstant(2),
    '*'
  );
  llvm::Value* expr2 = binaryOperation(
    numericConstant(8),
    expr1,
    '+'
  );
  llvm::Value* assgn1 = assignmentStatement("a", expr2);

  /*
   * b = a / 4
   */
  llvm::Value* expr3 = binaryOperation(
    variableValue("a"),
    numericConstant(4),
    '/'
  );
  llvm::Value* assgn2 = assignmentStatement("b", expr3);

  llvm::Value* ifElse = ifElseStatement();

  /*
   * Generate return instruction, verify, and print IR.
   */
  TheBuilder.CreateRet(variableValue("c"));
  llvm::verifyFunction(*targetFn);
  TheModule->print(llvm::outs(), NULL);

  int ret = generateMachineCode("target.o");

  delete TheModule;
  return ret;
}
