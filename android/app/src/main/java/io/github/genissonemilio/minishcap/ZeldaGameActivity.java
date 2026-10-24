package io.github.genissonemilio.minishcap;

import org.gbarecomp.GbaGameActivity;
import android.content.SharedPreferences;
import android.content.Intent;
import android.graphics.Bitmap;
import android.widget.Toast;
import java.io.ByteArrayOutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.ArrayList;
import java.util.Arrays;

public class ZeldaGameActivity extends GbaGameActivity {
    private final ExecutorService captureWorker=Executors.newSingleThreadExecutor();
    private final AtomicBoolean captureBusy=new AtomicBoolean();
    private boolean albumOpening;
    @Override public void onResume() { super.onResume();albumOpening=false; }
    // JNI copies one clean, color-corrected frame. PNG encoding stays off the game thread.
    public void captureFrame(byte[] rgb,int width,int height,int area,int room,int x,int y) {
        if(rgb==null || width<1 || height<1 || width>1024 || height>1024 || rgb.length!=width*height*3 ||
                isFinishing() || isDestroyed() || !captureBusy.compareAndSet(false,true)) return;
        captureWorker.execute(()->{
            try {
                int[] pixels=new int[width*height];
                for(int n=0;n<pixels.length;n++) pixels[n]=0xff000000 | ((rgb[n*3]&255)<<16) |
                    ((rgb[n*3+1]&255)<<8) | (rgb[n*3+2]&255);
                Bitmap image=Bitmap.createBitmap(pixels,width,height,Bitmap.Config.ARGB_8888);
                ByteArrayOutputStream png=new ByteArrayOutputStream();
                try { if(!image.compress(Bitmap.CompressFormat.PNG,100,png)) throw new java.io.IOException("Falha ao criar PNG"); }
                finally { image.recycle(); }
                new CaptureStore(getFilesDir()).save(png.toByteArray(),width,height,area,room,x,y);
                runOnUiThread(()->Toast.makeText(this,"Local marcado no álbum",Toast.LENGTH_SHORT).show());
            } catch(Exception error) {
                runOnUiThread(()->Toast.makeText(this,"Captura não salva: "+error.getMessage(),Toast.LENGTH_LONG).show());
            } finally { captureBusy.set(false); }
        });
    }
    public void showAlbum() {
        runOnUiThread(()->{
            if(!albumOpening && !isFinishing() && !isDestroyed()) {
                albumOpening=true;
                startActivity(new Intent(this,ZeldaAlbumActivity.class).putExtra("from_game",true));
            }
        });
    }
    @Override protected void onDestroy() { captureWorker.shutdown();super.onDestroy(); }
    // Called by the native settings menu on its game thread; preferences are thread-safe.
    public void persistPixelMode(int mode) {
        getSharedPreferences("minish",MODE_PRIVATE).edit().putInt("pixel_mode",mode==0?0:1).apply();
    }
    @Override protected String[] getArguments() {
        SharedPreferences p=getSharedPreferences("minish",MODE_PRIVATE);
        ArrayList<String> args=new ArrayList<>(Arrays.asList(
            "--rom",getFilesDir()+"/roms/minishcap_usa.gba",
            "--bios",getFilesDir()+"/bios/open_gba_bios.firmware",
            "--save",getFilesDir()+"/saves/minishcap.sav",
            "--window","--bios-skip-intro",
            "--volume",p.getBoolean("sound",true)?"100":"0",
            "--touch-size",String.valueOf(p.getInt("touch_size",100)),
            "--touch-opacity",String.valueOf(p.getInt("touch_opacity",70)),
            "--pixel-mode",String.valueOf(p.getInt("pixel_mode",1)),
            "variants/minishcap/game.toml"));
        // ADB QA builds alone expose the local observer and dummy audio sink.
        if ((getApplicationInfo().flags & android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE)!=0
                && getIntent().getBooleanExtra("qa",false)) {
            args.add("--tcp-observe"); args.add("19842"); args.add("--qa-muted");
            if(getIntent().getBooleanExtra("qa_strict",false)) args.add("--qa-strict");
        }
        return args.toArray(new String[0]);
    }
}
