; ModuleID = 'CWE_buf_loop.ll'
source_filename = "CWE_buf_loop.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

@.str = private unnamed_addr constant [6 x i8] c"%lld\0A\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @printLongLongLine(i64 noundef %value) #0 {
entry:
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i64 noundef %value)
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @CWE121_Stack_Based_Buffer_Overflow__CWE805_int64_t_declare_loop_01_bad() #0 {
entry:
  %dataBadBuffer = alloca [50 x i64], align 8
  %source = alloca [100 x i64], align 8
  %arraydecay = getelementptr inbounds [50 x i64], ptr %dataBadBuffer, i64 0, i64 0
  call void @llvm.memset.p0.i64(ptr align 8 %source, i8 0, i64 800, i1 false)
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %i.0, 100
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds [100 x i64], ptr %source, i64 0, i64 %i.0
  %0 = load i64, ptr %arrayidx, align 8
  %arrayidx1 = getelementptr inbounds i64, ptr %arraydecay, i64 %i.0
  store i64 %0, ptr %arrayidx1, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %i.0, 1
  br label %for.cond, !llvm.loop !5

for.end:                                          ; preds = %for.cond
  %arrayidx2 = getelementptr inbounds i64, ptr %arraydecay, i64 0
  %1 = load i64, ptr %arrayidx2, align 8
  call void @printLongLongLine(i64 noundef %1)
  %arrayidx3 = getelementptr inbounds i64, ptr %arraydecay, i64 49
  %2 = load i64, ptr %arrayidx3, align 8
  call void @printLongLongLine(i64 noundef %2)
  %arrayidx4 = getelementptr inbounds i64, ptr %arraydecay, i64 50
  %3 = load i64, ptr %arrayidx4, align 8
  call void @printLongLongLine(i64 noundef %3)
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #2

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @main() #0 {
entry:
  call void @CWE121_Stack_Based_Buffer_Overflow__CWE805_int64_t_declare_loop_01_bad()
  ret i32 0
}

attributes #0 = { noinline nounwind ssp uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }
attributes #2 = { nocallback nofree nounwind willreturn memory(argmem: write) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 1}
!3 = !{i32 7, !"frame-pointer", i32 1}
!4 = !{!"Homebrew clang version 16.0.6"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}