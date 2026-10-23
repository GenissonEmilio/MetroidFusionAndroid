package io.github.genissonemilio.minishcap;
import java.io.*;
import java.nio.file.*;
import java.util.Arrays;
import java.util.zip.*;

/** Runs on the host JVM without the Android SDK or a copyrighted game in CI. */
public class RomImporterTest {
    static int cases;
    static void reject(byte[] input,File target) throws Exception {
        byte[] before=Files.readAllBytes(target.toPath());
        try { RomImporter.importRom(new ByteArrayInputStream(input),target); throw new AssertionError("accepted invalid ROM"); }
        catch(IOException expected) { }
        if(!Arrays.equals(before,Files.readAllBytes(target.toPath()))) throw new AssertionError("replaced prior ROM on failure");
        if(target.getParentFile().listFiles().length!=1) throw new AssertionError("left temporary import files");
        cases++;
    }
    static byte[] zip(String name,byte[] content) throws Exception {
        ByteArrayOutputStream bytes=new ByteArrayOutputStream();
        try(ZipOutputStream out=new ZipOutputStream(bytes)) { out.putNextEntry(new ZipEntry(name)); out.write(content); out.closeEntry(); }
        return bytes.toByteArray();
    }
    public static void main(String[] args) throws Exception {
        Path folder=Files.createTempDirectory("minish-import-test");
        File dest=folder.resolve("game.gba").toFile();
        try {
            Files.write(dest.toPath(),new byte[]{1,2,3});
            reject(new byte[0],dest);
            reject(new byte[16777216],dest); // Correct size alone is insufficient.
            reject(new byte[16777217],dest);
            reject(zip("../../outside.gba",new byte[16]),dest);
            reject(zip("readme.txt",new byte[]{1}),dest);
            reject(zip("padding.txt",new byte[32*1024*1024+1]),dest);
            reject(new byte[]{'P','K',3,4,0,0},dest);
            if(args.length>0) {
                try(InputStream in=new FileInputStream(args[0])) { RomImporter.importRom(in,dest); }
                if(dest.length()!=RomImporter.SIZE) throw new AssertionError("valid owner ROM import failed");
                cases++;
            }
            System.out.println(cases+" import checks passed");
        } finally { for(File f:folder.toFile().listFiles()) f.delete(); Files.delete(folder); }
    }
}
