; ModuleID = 'test/Loop.ll'
source_filename = "test/Loop.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @populate(ptr noundef %0, ptr noundef %1, ptr noundef %2) #0 {
  br label %4

4:                                                ; preds = %9, %3
  %.02 = phi i32 [ 0, %3 ], [ %10, %9 ]
  %5 = icmp slt i32 %.02, 1000
  br i1 %5, label %6, label %11

6:                                                ; preds = %4
  %7 = sext i32 %.02 to i64
  %8 = getelementptr inbounds i32, ptr %0, i64 %7
  store i32 %.02, ptr %8, align 4
  br label %9

9:                                                ; preds = %6
  %10 = add nsw i32 %.02, 1
  br label %4, !llvm.loop !6

11:                                               ; preds = %4
  br label %12

12:                                               ; preds = %21, %11
  %.01 = phi i32 [ 0, %11 ], [ %22, %21 ]
  %13 = icmp slt i32 %.01, 1000
  br i1 %13, label %14, label %23

14:                                               ; preds = %12
  %15 = sext i32 %.01 to i64
  %16 = getelementptr inbounds i32, ptr %0, i64 %15
  %17 = load i32, ptr %16, align 4
  %18 = mul nsw i32 %17, 2
  %19 = sext i32 %.01 to i64
  %20 = getelementptr inbounds i32, ptr %1, i64 %19
  store i32 %18, ptr %20, align 4
  br label %21

21:                                               ; preds = %14
  %22 = add nsw i32 %.01, 1
  br label %12, !llvm.loop !8

23:                                               ; preds = %12
  br label %24

24:                                               ; preds = %33, %23
  %.0 = phi i32 [ 0, %23 ], [ %34, %33 ]
  %25 = icmp slt i32 %.0, 1000
  br i1 %25, label %26, label %35

26:                                               ; preds = %24
  %27 = sext i32 %.0 to i64
  %28 = getelementptr inbounds i32, ptr %1, i64 %27
  %29 = load i32, ptr %28, align 4
  %30 = add nsw i32 %29, 1
  %31 = sext i32 %.0 to i64
  %32 = getelementptr inbounds i32, ptr %2, i64 %31
  store i32 %30, ptr %32, align 4
  br label %33

33:                                               ; preds = %26
  %34 = add nsw i32 %.0, 1
  br label %24, !llvm.loop !9

35:                                               ; preds = %24
  ret void
}

attributes #0 = { noinline nounwind ssp uwtable(sync) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "probe-stack"="__chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 14, i32 5]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"Apple clang version 15.0.0 (clang-1500.3.9.4)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
