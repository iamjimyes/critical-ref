set(LLVM_LINK_COMPONENTS support)

add_clang_executable(critical-ref
  CriticalRef.cpp
  )
target_link_libraries(critical-ref
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )