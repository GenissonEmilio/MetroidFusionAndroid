package io.github.genissonemilio.minishcap;

import java.io.*;
import java.util.*;
import java.util.concurrent.CopyOnWriteArrayList;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

/** Private, bounded album. PNG publication is the commit point for each entry. */
public final class CaptureStore {
    public static final int LIMIT=200;
    private static final List<Runnable> listeners=new CopyOnWriteArrayList<>();
    private final File directory;
    public CaptureStore(File files) { directory=new File(files,"captures"); }
    public static void listen(Runnable r) { listeners.add(r); }
    public static void unlisten(Runnable r) { listeners.remove(r); }
    private static void changed() { for(Runnable r:listeners) r.run(); }
    private File file(String id,String extension) throws IOException {
        if(!id.matches("[0-9]{13}_[a-f0-9]{8}")) throw new IOException("Identificador inválido");
        return new File(directory,id+extension);
    }
    public static final class Entry {
        public final String id,title,note;
        public final File png;
        public final long time;
        public final int width,height,area,room,x,y;
        Entry(String id,File png,Properties p) {
            this.id=id; this.png=png; title=p.getProperty("title","Local marcado"); note=p.getProperty("note","");
            time=Long.parseLong(p.getProperty("time")); width=Integer.parseInt(p.getProperty("width"));
            height=Integer.parseInt(p.getProperty("height")); area=Integer.parseInt(p.getProperty("area"));
            room=Integer.parseInt(p.getProperty("room")); x=Integer.parseInt(p.getProperty("x")); y=Integer.parseInt(p.getProperty("y"));
        }
    }
    private Properties properties(String id) throws IOException {
        Properties p=new Properties();
        try(InputStream in=new FileInputStream(file(id,".properties"))) { p.load(in); }
        return p;
    }
    public synchronized List<Entry> list() {
        List<Entry> result=new ArrayList<>(); File[] files=directory.listFiles();
        if(files!=null) for(File f:files) {
            if(!f.getName().endsWith(".png")) continue;
            String id=f.getName().substring(0,f.getName().length()-4);
            try {
                Entry e=new Entry(id,file(id,".png"),properties(id));
                if(e.width>0 && e.height>0 && e.width<=1024 && e.height<=1024 && f.length()<=4*1024*1024)
                    result.add(e);
            } catch(IOException | NumberFormatException ignored) { /* incomplete/invalid entry */ }
        }
        result.sort((a,b)->b.id.compareTo(a.id)); return result;
    }
    private void writeProperties(String id,Properties p) throws IOException {
        File temporary=file(id,".meta.tmp"), destination=file(id,".properties");
        try {
            try(FileOutputStream out=new FileOutputStream(temporary)) { p.store(out,"Minish Cap album"); out.getFD().sync(); }
            try {
                Files.move(temporary.toPath(),destination.toPath(),
                    StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
            } catch(AtomicMoveNotSupportedException unsupported) {
                Files.move(temporary.toPath(),destination.toPath(),StandardCopyOption.REPLACE_EXISTING);
            }
        } finally { temporary.delete(); }
    }
    public synchronized Entry save(byte[] png,int width,int height,int area,int room,int x,int y) throws IOException {
        if(width<1 || height<1 || width>1024 || height>1024 || png.length<33 || png.length>4*1024*1024 ||
                png[0]!=(byte)137 || png[1]!=80 || png[2]!=78 || png[3]!=71)
            throw new IOException("Captura inválida");
        if(!directory.isDirectory() && !directory.mkdirs()) throw new IOException("Não foi possível criar o álbum");
        if(list().size()>=LIMIT) throw new IOException("Álbum cheio (200 locais). Exclua uma captura para continuar.");
        long time=System.currentTimeMillis();
        String id=time+"_"+UUID.randomUUID().toString().substring(0,8);
        Properties p=new Properties(); p.setProperty("title","Local marcado"); p.setProperty("note","");
        String[] keys={"width","height","area","room","x","y"}; int[] values={width,height,area,room,x,y};
        for(int n=0;n<keys.length;n++) p.setProperty(keys[n],Integer.toString(values[n]));
        p.setProperty("time",Long.toString(time));
        File temporary=file(id,".png.tmp"), image=file(id,".png");
        boolean committed=false;
        try {
            try(FileOutputStream out=new FileOutputStream(temporary)) { out.write(png); out.getFD().sync(); }
            writeProperties(id,p);
            if(!temporary.renameTo(image)) throw new IOException("Não foi possível salvar a captura");
            committed=true;
        } finally {
            temporary.delete(); if(!committed) file(id,".properties").delete();
        }
        changed(); return new Entry(id,image,p);
    }
    public synchronized void annotate(String id,String title,String note) throws IOException {
        if(!file(id,".png").isFile()) throw new IOException("Captura não encontrada");
        if(title.length()>80 || note.length()>1000) throw new IOException("Anotação muito longa");
        Properties p=properties(id); p.setProperty("title",title.trim().isEmpty()?"Local marcado":title.trim());
        p.setProperty("note",note.trim()); writeProperties(id,p); changed();
    }
    public synchronized void delete(String id) throws IOException {
        File image=file(id,".png");
        if(image.exists() && !image.delete()) throw new IOException("Não foi possível excluir a captura");
        file(id,".properties").delete(); changed();
    }
}
