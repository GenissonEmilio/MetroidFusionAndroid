package io.github.genissonemilio.metroidfusion;

import android.app.*;
import android.os.Bundle;
import android.content.*;
import android.graphics.*;
import android.graphics.drawable.*;
import android.text.InputFilter;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.*;

/** Host extension of the START menu; Back returns to the same game/menu. */
public class FusionAlbumActivity extends Activity {
    private static final int GOLD=0xff68c8f0,TEXT=0xffe4f4ff,MUTED=0xff9caed4,EXPORT=21;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private CaptureStore store;
    private LinearLayout entries;
    private TextView count,title,detail;
    private PixelImageView preview;
    private CaptureStore.Entry selected;
    private File exporting;
    private Button annotate,export,remove;
    private volatile int generation;
    private final Map<String,View> cards=new HashMap<>();
    private final Runnable changed=()->runOnUiThread(this::reload);
    private int dp(float n) { return Math.round(n*getResources().getDisplayMetrics().density); }
    private TextView text(String s,int size,int color) {
        TextView t=new TextView(this);t.setText(s);t.setTextSize(size);t.setTextColor(color);return t;
    }
    private GradientDrawable shape(int color,int border) {
        GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(10));
        d.setStroke(dp(1),border);return d;
    }
    private Button button(String label,Runnable action) {
        Button b=new Button(this);b.setText(label);b.setTextColor(GOLD);b.setTextSize(12);b.setAllCaps(false);
        b.setBackground(shape(0xff142850,0xff386090));b.setOnClickListener(v->action.run());return b;
    }
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        store=new CaptureStore(getFilesDir());
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new GradientDrawable(GradientDrawable.Orientation.TL_BR,new int[]{0xff182850,0xff080c20}));
        root.setPadding(dp(16),dp(8),dp(16),dp(8));root.setFitsSystemWindows(true);
        LinearLayout bar=new LinearLayout(this);bar.setGravity(Gravity.CENTER_VERTICAL);
        ImageView crest=new ImageView(this);crest.setImageResource(R.drawable.ic_crest);
        bar.addView(crest,new LinearLayout.LayoutParams(dp(42),dp(42)));
        LinearLayout heading=new LinearLayout(this);heading.setOrientation(LinearLayout.VERTICAL);heading.setPadding(dp(10),0,0,0);
        TextView name=text("Álbum da BSL",22,GOLD);name.setTypeface(Typeface.create("sans-serif-condensed",Typeface.BOLD));heading.addView(name);
        count=text("Registros da missão na BSL",11,MUTED);heading.addView(count);
        bar.addView(heading,new LinearLayout.LayoutParams(0,dp(56),1));
        bar.addView(button(getIntent().getBooleanExtra("from_game",false)?"Voltar ao jogo":"Voltar",this::finish),
            new LinearLayout.LayoutParams(dp(140),dp(44)));root.addView(bar);
        LinearLayout content=new LinearLayout(this);
        ScrollView scroll=new ScrollView(this);entries=new LinearLayout(this);entries.setOrientation(LinearLayout.VERTICAL);
        entries.setPadding(0,dp(5),dp(10),0);scroll.addView(entries);
        content.addView(scroll,new LinearLayout.LayoutParams(dp(220),-1));
        LinearLayout viewing=new LinearLayout(this);viewing.setOrientation(LinearLayout.VERTICAL);
        title=text("Seu diário de viagem",17,GOLD);title.setMaxLines(1);viewing.addView(title);
        preview=new PixelImageView(this);viewing.addView(preview,new LinearLayout.LayoutParams(-1,0,1));
        detail=text("Marque um local pelo botão de câmera durante a partida.",11,MUTED);detail.setMaxLines(3);
        detail.setPadding(0,dp(4),0,dp(4));viewing.addView(detail);
        LinearLayout actions=new LinearLayout(this);
        annotate=button("Anotar",this::annotate);export=button("Exportar PNG",this::export);remove=button("Excluir",this::remove);
        for(Button b:new Button[]{annotate,export,remove}) {
            LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(0,dp(42),1);p.setMargins(dp(3),0,dp(3),0);actions.addView(b,p);
        }
        viewing.addView(actions);content.addView(viewing,new LinearLayout.LayoutParams(0,-1,1));
        root.addView(content,new LinearLayout.LayoutParams(-1,0,1));setContentView(root);
        if(state!=null) { String path=state.getString("exporting");if(path!=null) exporting=new File(path); }
    }
    @Override protected void onStart() { super.onStart();CaptureStore.listen(changed);reload(); }
    @Override protected void onStop() { CaptureStore.unlisten(changed);generation++;super.onStop(); }
    @Override protected void onSaveInstanceState(Bundle state) {
        if(exporting!=null) state.putString("exporting",exporting.getAbsolutePath());super.onSaveInstanceState(state);
    }
    private void reload() {
        if(isFinishing() || isDestroyed()) return;
        final int token=++generation; final String old=selected==null?null:selected.id;
        worker.execute(()->{
            List<CaptureStore.Entry> list=store.list();
            runOnUiThread(()->{
                if(isFinishing() || isDestroyed() || token!=generation) return;
                entries.removeAllViews();cards.clear();count.setText(list.size()+" / "+CaptureStore.LIMIT+" locais marcados");
                CaptureStore.Entry choice=null;
                for(CaptureStore.Entry e:list) {
                    String date=new SimpleDateFormat("dd MMM · HH:mm",new Locale("pt","BR")).format(new Date(e.time));
                    LinearLayout card=new LinearLayout(this);card.setGravity(Gravity.CENTER_VERTICAL);
                    card.setPadding(dp(6),dp(6),dp(6),dp(6));card.setBackground(shape(0xff142850,0xff386090));
                    card.setFocusable(true);card.setClickable(true);card.setContentDescription(e.title+", "+date);
                    card.setOnClickListener(v->select(e));
                    ImageView thumb=new ImageView(this);thumb.setScaleType(ImageView.ScaleType.FIT_CENTER);
                    card.addView(thumb,new LinearLayout.LayoutParams(dp(72),dp(56)));
                    TextView label=text(e.title+"\n"+date,12,GOLD);label.setMaxLines(4);label.setPadding(dp(8),0,0,0);
                    card.addView(label,new LinearLayout.LayoutParams(0,-2,1));
                    LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,dp(86));p.bottomMargin=dp(8);entries.addView(card,p);
                    cards.put(e.id,card);
                    worker.execute(()->{
                        if(token!=generation) return;
                        Bitmap image=decode(e.png,4);
                        runOnUiThread(()->{
                            if(!isDestroyed() && token==generation && image!=null) {
                                BitmapDrawable drawable=new BitmapDrawable(getResources(),image);drawable.setFilterBitmap(false);thumb.setImageDrawable(drawable);
                            }
                        });
                    });
                    if(e.id.equals(old)) choice=e;
                }
                if(choice==null && !list.isEmpty()) choice=list.get(0);
                select(choice);
                if(list.isEmpty()) {
                    TextView empty=text("Ainda sem lembranças\n\nUse a câmera para marcar uma passagem, um tanque ou uma área para explorar depois.\n\nAs capturas ficam neste aparelho.",14,TEXT);
                    empty.setPadding(dp(12),dp(18),dp(12),0);entries.addView(empty);
                }
            });
        });
    }
    private void select(CaptureStore.Entry entry) {
        selected=entry;annotate.setEnabled(entry!=null);export.setEnabled(entry!=null);remove.setEnabled(entry!=null);
        for(Map.Entry<String,View> card:cards.entrySet())
            card.getValue().setBackground(shape(0xff142850,entry!=null && card.getKey().equals(entry.id)?GOLD:0xff386090));
        preview.setImage(null);
        if(entry==null) { title.setText("Seu diário de viagem");detail.setText("Marque um local pelo botão de câmera durante a partida.");return; }
        title.setText(entry.title);
        String date=new SimpleDateFormat("dd MMM yyyy · HH:mm",new Locale("pt","BR")).format(new Date(entry.time));
        detail.setText(date+(entry.note.isEmpty()?"\nDois dedos: ampliar · Arraste: mover · Dois toques: ampliar/restaurar":"\n"+entry.note));
        worker.execute(()->{
            final Bitmap result=decode(entry.png,1);
            runOnUiThread(()->{if(!isDestroyed() && selected!=null && selected.id.equals(entry.id)) preview.setImage(result);});
        });
    }
    private Bitmap decode(File image,int sample) {
        BitmapFactory.Options options=new BitmapFactory.Options();options.inJustDecodeBounds=true;
        BitmapFactory.decodeFile(image.getPath(),options);
        if(options.outWidth<1 || options.outWidth>1024 || options.outHeight<1 || options.outHeight>1024) return null;
        options.inJustDecodeBounds=false;options.inSampleSize=sample;
        return BitmapFactory.decodeFile(image.getPath(),options);
    }
    private void error(String s) { Toast.makeText(this,s,Toast.LENGTH_LONG).show(); }
    private void annotate() {
        if(selected==null) return;final CaptureStore.Entry entry=selected;
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);panel.setPadding(dp(20),dp(8),dp(20),0);
        EditText label=new EditText(this);label.setHint("Nome do local");label.setSingleLine(true);label.setText(entry.title);
        label.setFilters(new InputFilter[]{new InputFilter.LengthFilter(80)});panel.addView(label);
        EditText note=new EditText(this);note.setHint("O que você quer lembrar aqui?");note.setText(entry.note);note.setMaxLines(4);
        note.setFilters(new InputFilter[]{new InputFilter.LengthFilter(1000)});panel.addView(note);
        new AlertDialog.Builder(this).setTitle("Marcar esta lembrança").setView(panel)
            .setNegativeButton("Cancelar",null).setPositiveButton("Salvar",(d,w)->{
                final String enteredTitle=label.getText().toString(),enteredNote=note.getText().toString();
                worker.execute(()->{
                try { store.annotate(entry.id,enteredTitle,enteredNote); }
                catch(IOException e) { runOnUiThread(()->error(e.getMessage())); }
            });}).show();
    }
    private void remove() {
        if(selected==null) return;final CaptureStore.Entry entry=selected;
        new AlertDialog.Builder(this).setTitle("Excluir captura?").setMessage(entry.title+"\nA imagem e sua anotação serão removidas deste álbum.")
            .setNegativeButton("Cancelar",null).setPositiveButton("Excluir",(d,w)->worker.execute(()->{
                try { store.delete(entry.id); } catch(IOException e) { runOnUiThread(()->error(e.getMessage())); }
            })).show();
    }
    private void export() {
        if(selected==null) return;exporting=selected.png;
        Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("image/png").addCategory(Intent.CATEGORY_OPENABLE);
        i.putExtra(Intent.EXTRA_TITLE,"MetroidFusion-"+selected.id+".png");startActivityForResult(i,EXPORT);
    }
    @Override protected void onActivityResult(int request,int result,Intent data) {
        super.onActivityResult(request,result,data);
        if(request!=EXPORT || result!=RESULT_OK || data==null || data.getData()==null || exporting==null) return;
        final File source=exporting;final android.net.Uri destination=data.getData();exporting=null;
        worker.execute(()->{
            try(InputStream in=new FileInputStream(source);OutputStream out=getContentResolver().openOutputStream(destination,"wt")) {
                if(out==null) throw new IOException("Destino indisponível");
                byte[] buffer=new byte[8192];int n;while((n=in.read(buffer))!=-1) out.write(buffer,0,n);
                runOnUiThread(()->error("Captura exportada"));
            } catch(IOException e) { runOnUiThread(()->error("Não foi possível exportar a captura")); }
        });
    }
    @Override protected void onDestroy() { CaptureStore.unlisten(changed);generation++;worker.shutdown();super.onDestroy(); }
}
