/*
 * Xcode 26 adds a load command for libmetalirconverter.dylib even when no
 * converter entry points are used.  UE 5.6 does not ship that library in its
 * prebuilt macOS runtime.  This intentionally empty compatibility library
 * satisfies the loader until the engine package is updated.
 */
void RA4MetalIRConverterCompatibilityShim(void)
{
}

/* The game does not request offline Metal IR conversion.  These exports only
 * keep the Xcode 26 loader dependency satisfied for the local Development
 * build. */
void* IRCompilerCreate(void) { return 0; }
void IRCompilerDestroy(void) {}
void* IRMetalLibBinaryCreate(void) { return 0; }
void IRMetalLibBinaryDestroy(void) {}
void* IRMetalLibGetBytecode(void) { return 0; }
unsigned long long IRMetalLibGetBytecodeSize(void) { return 0; }
void* IRMetalLibSynthesizeStageInFunction(void) { return 0; }
void* IRShaderReflectionCreateFromJSON(void) { return 0; }
void IRShaderReflectionDestroy(void) {}
