package io.github.genissonemilio.minishcap;

import org.gbarecomp.GbaGameActivity;
import android.content.SharedPreferences;
import java.util.ArrayList;
import java.util.Arrays;

public class ZeldaGameActivity extends GbaGameActivity {
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
