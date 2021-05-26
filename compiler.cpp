#include <iostream>
#include <map>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Value.h"

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
    default:
      std::cerr << "Invalid operator: " << op << std::endl;
      return NULL;
  }
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
  }
}

int main(int argc, char const *argv[]) {
  TheModule = new llvm::Module("LLVM_Demo", TheContext);

  /*
   * Set up function signature and target() function.
   */
  llvm::FunctionType* targetFnSignature = llvm::FunctionType::get(
    llvm::Type::getVoidTy(TheContext),
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
   * 8 + 4 * 2
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

  /*
   * Generate return instruction, verify, and print IR.
   */
  TheBuilder.CreateRetVoid();
  llvm::verifyFunction(*targetFn);
  TheModule->print(llvm::outs(), NULL);

  delete TheModule;
  return 0;
}
