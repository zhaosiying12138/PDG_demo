; ModuleID = 'test_tmp.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @zsy_test(i32 noundef %0) #0 {
START:
  %1 = add nsw i32 0, 0
  %a1 = add nsw i32 0, 0
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  store i32 0, i32* %3, align 4
  %4 = load i32, i32* %3, align 4
  %5 = add nsw i32 %4, 1
  store i32 %5, i32* %3, align 4
  br label %L1

L1:                                               ; preds = %START
  %6 = load i32, i32* %3, align 4
  %7 = add nsw i32 %6, 2
  store i32 %7, i32* %3, align 4
  %8 = load i32, i32* %2, align 4
  %9 = icmp slt i32 %8, 10
  br i1 %9, label %L2, label %L3

L2:                                               ; preds = %L1
  %10 = add nsw i32 0, 0
  %11 = load i32, i32* %3, align 4
  %12 = add nsw i32 %11, 3
  store i32 %12, i32* %3, align 4
  %13 = load i32, i32* %2, align 4
  %14 = icmp slt i32 %13, 20
  br i1 %14, label %L4, label %L5

L3:                                               ; preds = %L1
  %15 = add nsw i32 0, 0
  %16 = load i32, i32* %2, align 4
  %17 = icmp slt i32 %16, 30
  br i1 %17, label %L5, label %L7

L4:                                               ; preds = %L2
  %18 = add nsw i32 0, 0
  %19 = load i32, i32* %3, align 4
  %20 = add nsw i32 %19, 4
  store i32 %20, i32* %3, align 4
  br label %L6

L5:                                               ; preds = %L2, %L3
  %21 = add nsw i32 0, 0
  %22 = load i32, i32* %3, align 4
  %23 = add nsw i32 %22, 5
  store i32 %23, i32* %3, align 4
  br label %L6

L6:                                               ; preds = %L4, %L5
  %24 = add nsw i32 0, 0
  %25 = load i32, i32* %3, align 4
  %26 = add nsw i32 %25, 7
  store i32 %26, i32* %3, align 4
  br label %L7

L7:                                               ; preds = %L3, %L6
  %27 = add nsw i32 0, 0
  %28 = load i32, i32* %3, align 4
  %29 = add nsw i32 %28, 8
  store i32 %29, i32* %3, align 4
  %30 = load i32, i32* %3, align 4
  br label %STOP

STOP:                                             ; preds = %L7
  ret i32 %30
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 1}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{!"clang version 14.0.6 (https://github.com/llvm/llvm-project.git f28c006a5895fc0e329fe15fead81e37457cb1d1)"}
