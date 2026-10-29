package io.github.genissonemilio.metroidfusion;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class CaptureStoreTest {
    private static void check(boolean condition) { if(!condition) throw new AssertionError(); }
    private static void rejects(Throwing r) throws Exception {
        try { r.run();throw new AssertionError("Expected rejection"); } catch(IOException expected) { }
    }
    interface Throwing { void run() throws Exception; }
    public static void main(String[] args) throws Exception {
        Path root=Files.createTempDirectory("fusion-album-test");
        try {
            Path save=root.resolve("fusion.sav");Files.write(save,new byte[]{1,2,3});
            byte[] png=Base64.getDecoder().decode("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jN1sAAAAASUVORK5CYII=");
            CaptureStore store=new CaptureStore(root.toFile());check(store.list().isEmpty());
            rejects(()->store.save(new byte[40],1,1,0,0,0,0));
            rejects(()->store.save(png,1025,1,0,0,0,0));
            CaptureStore.Entry e=store.save(png,1,1,21,4,120,80);
            check(Arrays.equals(png,Files.readAllBytes(e.png.toPath())));
            store.annotate(e.id,"Entrada da floresta","Baú após a ponte. Voltar com a chave.");
            CaptureStore.Entry read=new CaptureStore(root.toFile()).list().get(0);
            check(read.title.equals("Entrada da floresta") && read.note.contains("Baú") && read.area==21 && read.x==120);
            rejects(()->store.annotate(e.id,"a".repeat(81),""));
            rejects(()->store.delete("../../fusion.sav"));
            rejects(()->store.annotate("../../fusion.sav","X","X"));
            // Incomplete writes and untrusted filenames never become entries.
            Files.write(root.resolve("captures/0000000000000_aaaaaaaa.png"),png);
            Files.write(root.resolve("captures/not-an-id.png"),png);
            check(store.list().size()==1);
            for(int n=1;n<CaptureStore.LIMIT;n++) store.save(png,1,1,-1,-1,-1,-1);
            check(store.list().size()==CaptureStore.LIMIT);
            rejects(()->store.save(png,1,1,0,0,0,0));
            store.delete(e.id);check(store.list().size()==CaptureStore.LIMIT-1);
            check(Arrays.equals(Files.readAllBytes(save),new byte[]{1,2,3}));
            try(var files=Files.list(root.resolve("captures"))) {
                check(files.noneMatch(p->p.toString().endsWith(".tmp")));
            }
            System.out.println("Album persistence, annotations, bounds, invalid entries and save isolation passed");
        } finally {
            try(var paths=Files.walk(root)) {
                paths.sorted(Comparator.reverseOrder()).forEach(p->{try{Files.delete(p);}catch(IOException e){throw new UncheckedIOException(e);}});
            }
        }
    }
}
