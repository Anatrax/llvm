define i32 @addRecursive(i32 %a, i32 %b) {
entry:
  %X = alloca i32
  %tmp1 = icmp eq i32 %a, 0
  br i1 %tmp1, label %done, label %recurse

recurse:
  %tmp2 = sub i32 %a, 1
  %tmp3 = add i32 %b, 1
  store i32 %tmp3 i32* %X
  %tmp4 = call i32 @addRecursive(i32 %tmp2, i32 %tmp3)
  %X1 = load float, float* %X
  ret i32 %tmp4

done:
  ret i32 %b
}
