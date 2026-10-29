package io.github.genissonemilio.metroidfusion;

import android.app.*;
import android.os.*;
import android.content.*;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class FusionSetupActivity extends Activity {
    private static final int PICK_ROM=10, EXPORT_SAVE=11;
    private static final int GOLD=0xff68c8f0, TEXT=0xffe4f4ff, MUTED=0xff9caed4;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private TextView status;
    private Button play,importButton,imageButton;
    private ProgressBar progress;
    private boolean busy;
    private int dp(float v) { return Math.round(v*getResources().getDisplayMetrics().density); }
    private File rom() { return new File(getFilesDir(),"roms/fusion_usa.gba"); }
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        build();
    }
    @Override protected void onResume() { super.onResume(); if(status!=null && !busy) update(); }
    private TextView text(String s,int size,int color) {
        TextView t=new TextView(this); t.setText(s); t.setTextSize(size); t.setTextColor(color);
        t.setLineSpacing(dp(3),1); return t;
    }
    private GradientDrawable shape(int fill,int stroke,int radius) {
        GradientDrawable d=new GradientDrawable(); d.setColor(fill); d.setCornerRadius(dp(radius));
        if(stroke!=0) d.setStroke(dp(1),stroke); return d;
    }
    private Button button(String label,boolean primary,Runnable action) {
        Button b=new Button(this); b.setText(label); b.setTextSize(14); b.setAllCaps(false);
        b.setTextColor(primary?0xff081838:TEXT);
        b.setBackground(shape(primary?GOLD:0xff142850,primary?0xffb0e8ff:0xff386090,12));
        b.setPadding(dp(16),0,dp(16),0); b.setMinHeight(dp(48));
        b.setOnClickListener(v->action.run());
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,dp(49)); p.bottomMargin=dp(10);
        b.setLayoutParams(p); return b;
    }
    private void build() {
        LinearLayout root=new LinearLayout(this); root.setOrientation(LinearLayout.HORIZONTAL);
        root.setGravity(Gravity.CENTER_VERTICAL); root.setPadding(dp(28),dp(12),dp(28),dp(12));
        root.setBackground(new GradientDrawable(GradientDrawable.Orientation.TL_BR,new int[]{0xff182850,0xff080c20}));
        root.setFitsSystemWindows(true);
        ScrollView heroScroll=new ScrollView(this);
        LinearLayout hero=new LinearLayout(this); hero.setOrientation(LinearLayout.VERTICAL);
        hero.setPadding(0,dp(8),dp(30),dp(8));
        ImageView crest=new ImageView(this); crest.setImageResource(R.drawable.ic_crest);
        hero.addView(crest,new LinearLayout.LayoutParams(dp(84),dp(84)));
        TextView eyebrow=text("METROID",12,GOLD); eyebrow.setLetterSpacing(.18f);
        hero.addView(eyebrow);
        TextView title=text("Metroid Fusion",34,TEXT); title.setTypeface(Typeface.create("sans-serif-condensed",Typeface.BOLD));
        hero.addView(title);
        TextView description=text("Explore a estação BSL.\nRegistre sua missão.",17,MUTED);
        description.setPadding(0,dp(10),0,dp(12)); hero.addView(description);
        hero.addView(text("ANDROID  •  ARM64\nControles touch · Saves locais",11,GOLD));
        heroScroll.addView(hero);
        root.addView(heroScroll,new LinearLayout.LayoutParams(0,-1,1f));
        ScrollView cardScroll=new ScrollView(this); cardScroll.setFillViewport(true);
        LinearLayout card=new LinearLayout(this); card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER_VERTICAL); card.setPadding(dp(22),dp(16),dp(22),dp(16));
        card.setBackground(shape(0xff101c40,0xff305888,20));
        card.addView(text("Prepare sua missão",21,TEXT));
        status=text("",13,MUTED); status.setPadding(0,dp(9),0,dp(12));
        status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE); card.addView(status);
        progress=new ProgressBar(this,null,android.R.attr.progressBarStyleHorizontal);
        progress.setIndeterminate(true); progress.setVisibility(View.GONE); card.addView(progress);
        play=button("Jogar",true,this::launchGame); card.addView(play);
        importButton=button("Importar ROM  ·  .gba ou .zip",false,this::pickRom); card.addView(importButton);
        imageButton=button("Imagem",false,this::imageSettings);
        LinearLayout extras=new LinearLayout(this);
        LinearLayout.LayoutParams extraHalf=new LinearLayout.LayoutParams(0,dp(49),1);extraHalf.rightMargin=dp(8);extraHalf.bottomMargin=dp(10);
        extras.addView(imageButton,extraHalf);
        extras.addView(button("Álbum da BSL",false,()->startActivity(new Intent(this,FusionAlbumActivity.class))),
            new LinearLayout.LayoutParams(0,dp(49),1));card.addView(extras);
        LinearLayout row=new LinearLayout(this);
        Button controls=button("Controles",false,this::controls);
        Button save=button("Exportar save",false,this::exportSave);
        LinearLayout.LayoutParams half=new LinearLayout.LayoutParams(0,dp(48),1); half.rightMargin=dp(8);
        row.addView(controls,half); row.addView(save,new LinearLayout.LayoutParams(0,dp(48),1)); card.addView(row);
        TextView about=text("Projeto de fãs independente • v"+BuildConfig.VERSION_NAME+"\nBIOS aberta incluída. Jogo original não incluído.",10,MUTED);
        about.setPadding(0,dp(12),0,0); about.setOnClickListener(v->about()); card.addView(about);
        cardScroll.addView(card); root.addView(cardScroll,new LinearLayout.LayoutParams(0,-1,1.2f));
        setContentView(root); update();
    }
    private void update() {
        if(imageButton!=null) imageButton.setText("Imagem  ·  "+
            (getSharedPreferences("fusion",MODE_PRIVATE).getInt("pixel_mode",1)==0?"Pixel perfeito":"Suave"));
        boolean ready=rom().isFile() && rom().length()==RomImporter.SIZE;
        play.setEnabled(ready && !busy); play.setAlpha(ready?1:.4f);
        importButton.setEnabled(!busy);
        if(!busy) status.setText(ready?"ROM USA verificada e salva no aparelho. Pode remover o arquivo original após a importação.":
            "Importe sua cópia de Metroid Fusion (USA). A verificação acontece no aparelho, sem envio de arquivos.");
    }
    private void pickRom() {
        Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);
        i.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"application/zip","application/octet-stream","application/x-gba-rom","application/x-zip-compressed"});
        startActivityForResult(i,PICK_ROM);
    }
    private void launchGame() {
        Intent i=new Intent(this,FusionGameActivity.class);
        // Exported launcher accepts only a boolean QA flag; no file paths/commands.
        if(getIntent().getBooleanExtra("qa",false)) {
            i.putExtra("qa",true);
            i.putExtra("qa_strict",getIntent().getBooleanExtra("qa_strict",false));
        }
        startActivity(i);
    }
    @Override protected void onActivityResult(int request,int result,Intent data) {
        super.onActivityResult(request,result,data);
        if(result!=RESULT_OK || data==null || data.getData()==null) return;
        Uri uri=data.getData();
        if(request==PICK_ROM) {
            busy=true; update(); status.setText("Importando e verificando a ROM…"); progress.setVisibility(View.VISIBLE);
            worker.execute(()->{
                String error=null;
                try(InputStream input=getContentResolver().openInputStream(uri)) {
                    if(input==null) throw new IOException("Não foi possível abrir esse arquivo.");
                    RomImporter.importRom(input,rom());
                } catch(Exception e) { error=e.getMessage(); }
                final String message=error;
                runOnUiThread(()->{
                    if(isFinishing() || isDestroyed()) return;
                    busy=false; progress.setVisibility(View.GONE); update();
                    if(message!=null) new AlertDialog.Builder(this).setTitle("Não foi possível importar")
                        .setMessage(message).setPositiveButton("Entendi",null).show();
                });
            });
        } else if(request==EXPORT_SAVE) {
            worker.execute(()->{
                try(InputStream in=new FileInputStream(new File(getFilesDir(),"saves/fusion.sav"));
                    OutputStream out=getContentResolver().openOutputStream(uri,"wt")) {
                    if(out==null) throw new IOException("Não foi possível abrir o destino.");
                    byte[] b=new byte[8192]; int n; while((n=in.read(b))!=-1) out.write(b,0,n);
                    runOnUiThread(()->Toast.makeText(this,"Save exportado",Toast.LENGTH_LONG).show());
                } catch(IOException e) { runOnUiThread(()->Toast.makeText(this,"Falha ao exportar save",Toast.LENGTH_LONG).show()); }
            });
        }
    }
    private void exportSave() {
        if(!new File(getFilesDir(),"saves/fusion.sav").isFile()) {
            new AlertDialog.Builder(this).setMessage("Salve seu progresso dentro do jogo primeiro. Os estados rápidos ficam no menu durante a partida.")
                .setPositiveButton("Entendi",null).show(); return;
        }
        Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/octet-stream");
        i.addCategory(Intent.CATEGORY_OPENABLE); i.putExtra(Intent.EXTRA_TITLE,"MetroidFusion.sav"); startActivityForResult(i,EXPORT_SAVE);
    }
    private void controls() {
        LinearLayout panel=new LinearLayout(this); panel.setOrientation(LinearLayout.VERTICAL); panel.setPadding(dp(24),dp(8),dp(24),0);
        panel.addView(text("Direcional: mover · A/B: ações\nL/R: ações secundárias · START: mapa\nNo mapa: R → Álbum da BSL → R → Status\nÁlbum: direcional seleciona · A abre a foto ou a câmera\nCâmera: A fotografa a última cena do jogo · B volta ao álbum\nL/R troca de aba · START fecha o menu\nAlgumas cenas não permitem abrir o mapa.\nMENU ou Voltar: pausa, estados rápidos e gerenciamento de fotos\nOs ícones são decorativos; as letras identificam os botões.\nControles Bluetooth/USB também são aceitos pelo runtime.",13,TEXT));
        slider(panel,"Tamanho", "touch_size",80,130,100,"%");
        slider(panel,"Opacidade", "touch_opacity",30,90,70,"%");
        android.content.SharedPreferences prefs=getSharedPreferences("fusion",MODE_PRIVATE);
        Switch sound=new Switch(this); sound.setText("Som na próxima partida"); sound.setTextColor(TEXT);
        sound.setPadding(0,dp(12),0,dp(8)); sound.setChecked(prefs.getBoolean("sound",true));
        sound.setOnCheckedChangeListener((v,on)->prefs.edit().putBoolean("sound",on).apply()); panel.addView(sound);
        ScrollView scroll=new ScrollView(this); scroll.addView(panel);
        new AlertDialog.Builder(this).setTitle("Controles da missão").setView(scroll).setPositiveButton("Pronto",null).show();
    }
    private void imageSettings() {
        android.content.SharedPreferences prefs=getSharedPreferences("fusion",MODE_PRIVATE);
        LinearLayout panel=new LinearLayout(this); panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(24),dp(8),dp(24),dp(12));
        panel.addView(text("Escolha o visual da missão",17,GOLD));
        RadioGroup modes=new RadioGroup(this);
        String[] labels={"Pixel perfeito\nPixels nítidos e uniformes, fiéis à arte original.",
            "Suave · Scale2x\nBordas mais arredondadas, sem borrar as cores."};
        for(int n=0;n<labels.length;++n) {
            RadioButton choice=new RadioButton(this); choice.setId(n+100);
            choice.setText(labels[n]); choice.setTextColor(TEXT); choice.setTextSize(14);
            choice.setPadding(0,dp(12),0,dp(12)); modes.addView(choice);
        }
        modes.check(prefs.getInt("pixel_mode",1)==0?100:101);
        modes.setOnCheckedChangeListener((group,id)->{
            prefs.edit().putInt("pixel_mode",id==100?0:1).apply(); update();
        });
        panel.addView(modes);
        TextView help=text("A proporção original é preservada nos dois modos. A ampliação usa passos inteiros; algumas telas podem mostrar margens.\n\nVale na próxima partida. Para comparar durante o jogo: MENU → Graphics → Estilo dos pixels.",12,MUTED);
        help.setPadding(0,dp(10),0,0); panel.addView(help);
        ScrollView scroll=new ScrollView(this); scroll.addView(panel);
        new AlertDialog.Builder(this).setTitle("Imagem e pixels").setView(scroll).setPositiveButton("Pronto",null).show();
    }
    private void slider(LinearLayout box,String label,String key,int min,int max,int fallback,String suffix) {
        android.content.SharedPreferences prefs=getSharedPreferences("fusion",MODE_PRIVATE);
        int value=prefs.getInt(key,fallback);
        TextView caption=text(label+": "+value+suffix,14,GOLD); caption.setPadding(0,dp(12),0,0); box.addView(caption);
        SeekBar bar=new SeekBar(this); bar.setMax(max-min); bar.setProgress(value-min); box.addView(bar);
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar b,int p,boolean user) {
                caption.setText(label+": "+(p+min)+suffix);
                if(user) prefs.edit().putInt(key,p+min).apply();
            }
            public void onStartTrackingTouch(SeekBar b) {} public void onStopTrackingTouch(SeekBar b) {}
        });
    }
    private void about() {
        new AlertDialog.Builder(this).setTitle("Metroid Fusion para Android")
            .setMessage("Base Android derivada de GenissonEmilio/ZeldaMinishCapAndroid, MinishCapRecomp e gbarecomp. Adaptação específica para Metroid Fusion.\n\nFirmware substituto Cult of GBA BIOS, licença MIT. Runtime e port seguem suas licenças originais.\n\nNão afiliado à Nintendo. Consulte LICENSE e ANDROID.md no repositório para créditos e código-fonte.")
            .setPositiveButton("Fechar",null).show();
    }
    @Override protected void onDestroy() { worker.shutdown(); super.onDestroy(); }
}
