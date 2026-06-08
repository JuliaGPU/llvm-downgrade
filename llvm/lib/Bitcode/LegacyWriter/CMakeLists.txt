add_llvm_component_library(LLVMLegacyBitWriter
  PointerRewriter.cpp
  BitWriter50.cpp
  BitcodeWriter50.cpp
  BitcodeWriterPass50.cpp
  ValueEnumerator50.cpp
  ModuleRewriter50.cpp
  BitWriter70.cpp
  BitcodeWriter70.cpp
  BitcodeWriterPass70.cpp
  ValueEnumerator70.cpp
  ModuleRewriter70.cpp
  BitcodeWriter140.cpp
  ValueEnumerator140.cpp
  ModuleRewriter140.cpp

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Analysis
  BitWriter
  Core
  MC
  Object
  Support
  TargetParser
  )
