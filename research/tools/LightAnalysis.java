// Pre-script: only the analyzers needed for functions (from .pdata) and decompilation; full auto analysis of the
// 68 MB image runs out of a 12 GB heap.
// analyzeHeadless <projects> sekiro -import sekiro_dump.exe -scriptPath <here> -preScript LightAnalysis.java (GHIDRA_MAXMEM=20G)
import ghidra.app.script.GhidraScript;

public class LightAnalysis extends GhidraScript {
  @Override
  public void run() throws Exception {
    String[] off = {"Decompiler Parameter ID", "Decompiler Switch Analysis", "Stack", "Call Convention ID", "Data Reference",
        "Embedded Media", "Function ID", "Reference", "Scalar Operand References", "Create Address Tables", "Shared Return Calls",
        "Aggressive Instruction Finder", "Non-Returning Functions - Discovered", "x86 Constant Reference Analyzer",
        "Variadic Function Signature Override", "Apply Data Archives", "ASCII Strings", "Function Start Search",
        "Condense Filler Bytes", "DWARF", "GCC Exception Handlers", "Decompiler Call-Fixup Analysis"};
    for (String o : off) {
      try { setAnalysisOption(currentProgram, o, "false"); println("off: " + o); }
      catch (Exception e) { println("no option: " + o); }
    }
  }
}
