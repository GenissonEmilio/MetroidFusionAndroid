package io.github.genissonemilio.metroidfusion;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.widget.Toast;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.text.Normalizer;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.atomic.AtomicBoolean;

/** Disk/PNG work for the in-game page. Never opens an Android screen. */
final class GameAlbumBridge implements AutoCloseable {
    private final Activity activity;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private final AtomicBoolean captureBusy=new AtomicBoolean();
    GameAlbumBridge(Activity activity) { this.activity=activity; }
    private CaptureStore store() { return new CaptureStore(activity.getFilesDir()); }
    private static native void nativePage(long token,int page,int total,
        String[] ids,String[] titles,byte[] thumbnails);
    private static native void nativePhoto(long token,byte[] rgb);
    private static native void nativeSaved(long token,boolean success);

    void loadPage(int page,long token) {
        if(page<0 || page>33) return;
        execute(()->{
            List<CaptureStore.Entry> all=store().list();
            int start=Math.min(page*6,all.size()), count=Math.min(6,all.size()-start);
            String[] ids=new String[count],titles=new String[count];
            byte[] thumbnails=new byte[count*60*36*3];
            for(int n=0;n<count;++n) {
                CaptureStore.Entry entry=all.get(start+n);
                ids[n]=entry.id;
                titles[n]=Normalizer.normalize(entry.title,Normalizer.Form.NFD)
                    .replaceAll("\\p{M}","").replaceAll("[^\\x20-\\x7E]","");
                byte[] image=decode(entry,60,36);
                if(image!=null) System.arraycopy(image,0,thumbnails,n*60*36*3,image.length);
            }
            nativePage(token,page,all.size(),ids,titles,thumbnails);
        });
    }
    void loadPhoto(String id,long token) {
        execute(()->{
            byte[] image=null;
            for(CaptureStore.Entry entry:store().list()) if(entry.id.equals(id)) {
                image=decode(entry,240,160);break;
            }
            nativePhoto(token,image);
        });
    }
    private static byte[] decode(CaptureStore.Entry entry,int width,int height) {
        BitmapFactory.Options options=new BitmapFactory.Options();options.inJustDecodeBounds=true;
        BitmapFactory.decodeFile(entry.png.getPath(),options);
        if(options.outWidth<1 || options.outWidth>1024 || options.outHeight<1 || options.outHeight>1024)
            return null;
        options.inJustDecodeBounds=false;
        Bitmap source=BitmapFactory.decodeFile(entry.png.getPath(),options);
        if(source==null) return null;
        Bitmap target=Bitmap.createBitmap(width,height,Bitmap.Config.ARGB_8888);
        try {
            Canvas canvas=new Canvas(target);canvas.drawColor(0xff1c4444);
            int dw=Math.min(width,height*source.getWidth()/source.getHeight());
            int dh=Math.min(height,width*source.getHeight()/source.getWidth());
            Paint paint=new Paint();paint.setFilterBitmap(false);paint.setAntiAlias(false);
            canvas.drawBitmap(source,null,new Rect((width-dw)/2,(height-dh)/2,
                (width+dw)/2,(height+dh)/2),paint);
            int[] pixels=new int[width*height];target.getPixels(pixels,0,width,0,0,width,height);
            byte[] rgb=new byte[width*height*3];
            for(int n=0;n<pixels.length;++n) {
                rgb[n*3]=(byte)(pixels[n]>>16);rgb[n*3+1]=(byte)(pixels[n]>>8);rgb[n*3+2]=(byte)pixels[n];
            }
            return rgb;
        } finally { source.recycle();target.recycle(); }
    }
    void save(byte[] rgb,int width,int height,int area,int room,int x,int y,long token) {
        if(rgb==null || width<1 || height<1 || width>1024 || height>1024 || rgb.length!=width*height*3 ||
                !captureBusy.compareAndSet(false,true)) {
            if(token!=0) nativeSaved(token,false);return;
        }
        if(!execute(()->{
            boolean success=false;
            try {
                int[] pixels=new int[width*height];
                for(int n=0;n<pixels.length;++n) pixels[n]=0xff000000 | ((rgb[n*3]&255)<<16) |
                    ((rgb[n*3+1]&255)<<8) | (rgb[n*3+2]&255);
                Bitmap image=Bitmap.createBitmap(pixels,width,height,Bitmap.Config.ARGB_8888);
                ByteArrayOutputStream png=new ByteArrayOutputStream();
                try { if(!image.compress(Bitmap.CompressFormat.PNG,100,png)) throw new IOException("Falha ao criar PNG"); }
                finally { image.recycle(); }
                store().save(png.toByteArray(),width,height,area,room,x,y);success=true;
            } catch(IOException | RuntimeException error) {
                if(token==0) toast("Captura não salva: "+error.getMessage());
            } finally {
                captureBusy.set(false);
                if(token!=0) nativeSaved(token,success);
                else if(success) toast("Local marcado no álbum");
            }
        })) {
            captureBusy.set(false);if(token!=0) nativeSaved(token,false);
        }
    }
    private boolean execute(Runnable job) {
        try { worker.execute(job);return true; }
        catch(RejectedExecutionException ignored) { return false; }
    }
    private void toast(String message) {
        activity.runOnUiThread(()->{
            if(!activity.isFinishing() && !activity.isDestroyed())
                Toast.makeText(activity,message,Toast.LENGTH_SHORT).show();
        });
    }
    @Override public void close() { worker.shutdown(); }
}
