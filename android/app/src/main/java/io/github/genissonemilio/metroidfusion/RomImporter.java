package io.github.genissonemilio.metroidfusion;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/** Bounded, transactional import: ZIP entries are streamed, never extracted by name. */
public final class RomImporter {
    public static final long SIZE = 8388608;
    public static final String SHA1 = "ca33f4348c2c05dd330d37b97e2c5a69531dfe87";
    private RomImporter() {}
    public static void importRom(InputStream source, File destination) throws IOException {
        File parent = destination.getParentFile();
        if (!parent.isDirectory() && !parent.mkdirs()) throw new IOException("Não foi possível criar a biblioteca.");
        File pending = File.createTempFile("import-", ".pending", parent);
        try (BufferedInputStream input = new BufferedInputStream(source)) {
            input.mark(4);
            int a=input.read(), b=input.read(); input.reset();
            if (a=='P' && b=='K') {
                try (ZipInputStream zip = new ZipInputStream(input)) {
                    boolean found=false;
                    int entries=0;
                    long skipped=0;
                    ZipEntry e;
                    byte[] buffer=new byte[65536];
                    while ((e=zip.getNextEntry()) != null) {
                        if (++entries>256) throw new IOException("ZIP com arquivos demais.");
                        String name=e.getName().toLowerCase(java.util.Locale.ROOT);
                        if (!e.isDirectory() && (name.endsWith(".gba") || name.endsWith(".agb"))) {
                            if (found) throw new IOException("Selecione um ZIP com apenas uma ROM de GBA.");
                            verifyAndCopy(zip,pending); found=true;
                        } else {
                            int n;
                            while ((n=zip.read(buffer))!=-1) {
                                skipped+=n;
                                if(skipped>32*1024*1024L) throw new IOException("Conteúdo extra do ZIP excede o limite.");
                            }
                        }
                        zip.closeEntry();
                    }
                    if(!found) throw new IOException("Nenhuma ROM .gba encontrada no ZIP.");
                }
            } else verifyAndCopy(input,pending);
            Files.move(pending.toPath(),destination.toPath(),StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);
        } finally { pending.delete(); }
    }
    private static void verifyAndCopy(InputStream input,File destination) throws IOException {
        MessageDigest digest;
        try { digest=MessageDigest.getInstance("SHA-1"); }
        catch(NoSuchAlgorithmException impossible) { throw new IOException(impossible); }
        long size=0;
        try(FileOutputStream output=new FileOutputStream(destination)) {
            byte[] data=new byte[65536]; int n;
            while((n=input.read(data))!=-1) {
                size+=n;
                if(size>SIZE) throw new IOException("A ROM deve ter exatamente 8 MB (USA).");
                digest.update(data,0,n); output.write(data,0,n);
            }
            output.getFD().sync();
        }
        StringBuilder hex=new StringBuilder();
        for(byte b:digest.digest()) hex.append(String.format("%02x",b&255));
        if(size!=SIZE || !SHA1.equals(hex.toString()))
            throw new IOException("ROM incompatível. Use Metroid Fusion (USA), original, sem patches (8 MB).");
    }
}
