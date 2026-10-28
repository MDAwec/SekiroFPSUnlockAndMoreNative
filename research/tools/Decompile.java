// Decompiles the functions containing the given addresses into decompiled.c next to this script.
// analyzeHeadless <projects> sekiro -process sekiro_dump.exe -noanalysis -readOnly -scriptPath <here> -postScript Decompile.java 140d9cd90 ...
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import java.io.*;

public class Decompile extends GhidraScript {
  @Override
  public void run() throws Exception {
    DecompInterface d = new DecompInterface();
    d.openProgram(currentProgram);
    try (PrintWriter out = new PrintWriter(new FileWriter(new File(getSourceFile().getParentFile().getFile(false), "decompiled.c"), true))) {
      for (String a : getScriptArgs()) {
        Function f = getFunctionContaining(toAddr(a));
        if (f == null) { out.println("// no function at " + a); continue; }
        DecompileResults r = d.decompileFunction(f, 120, monitor);
        out.println("// ==== " + a + " -> " + f.getEntryPoint() + " " + f.getName());
        out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
      }
    }
  }
}
