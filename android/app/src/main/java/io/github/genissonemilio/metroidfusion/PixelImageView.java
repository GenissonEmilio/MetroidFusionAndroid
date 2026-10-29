package io.github.genissonemilio.metroidfusion;

import android.content.Context;
import android.graphics.*;
import android.view.*;

/** Nearest-neighbor album viewer with pinch zoom, pan and double-tap reset. */
public final class PixelImageView extends View {
    private Bitmap image;
    private final Paint paint=new Paint();
    private float zoom=1,panX,panY,lastX,lastY;
    private final ScaleGestureDetector pinch;
    private final GestureDetector gestures;
    public PixelImageView(Context c) {
        super(c); setContentDescription("Captura do jogo. Use dois dedos para ampliar e arraste para explorar.");
        setFocusable(true); paint.setFilterBitmap(false); paint.setDither(false);
        pinch=new ScaleGestureDetector(c,new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override public boolean onScale(ScaleGestureDetector d) {
                zoom=Math.max(1,Math.min(6,zoom*d.getScaleFactor())); invalidate(); return true;
            }
        });
        gestures=new GestureDetector(c,new GestureDetector.SimpleOnGestureListener() {
            @Override public boolean onDown(MotionEvent e) { return true; }
            @Override public boolean onDoubleTap(MotionEvent e) { zoom=zoom>1?1:2;panX=panY=0;invalidate();return true; }
        });
    }
    public void setImage(Bitmap value) { image=value;zoom=1;panX=panY=0;invalidate(); }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas); canvas.drawColor(0xff071813);
        if(image==null) return;
        float fit=Math.min((float)getWidth()/image.getWidth(),(float)getHeight()/image.getHeight());
        if(fit>=1) fit=(float)Math.floor(fit);
        float w=image.getWidth()*fit*zoom,h=image.getHeight()*fit*zoom;
        panX=Math.max(-Math.max(0,(w-getWidth())/2),Math.min(Math.max(0,(w-getWidth())/2),panX));
        panY=Math.max(-Math.max(0,(h-getHeight())/2),Math.min(Math.max(0,(h-getHeight())/2),panY));
        float x=(getWidth()-w)/2+panX,y=(getHeight()-h)/2+panY;
        canvas.drawBitmap(image,null,new RectF(x,y,x+w,y+h),paint);
    }
    @Override public boolean onTouchEvent(MotionEvent e) {
        pinch.onTouchEvent(e); gestures.onTouchEvent(e);
        if(e.getActionMasked()==MotionEvent.ACTION_DOWN) { lastX=e.getX();lastY=e.getY(); }
        else if(e.getActionMasked()==MotionEvent.ACTION_MOVE) {
            if(!pinch.isInProgress() && e.getPointerCount()==1 && zoom>1) {
                panX+=e.getX()-lastX;panY+=e.getY()-lastY;invalidate();
            }
            lastX=e.getX();lastY=e.getY();
        } else if(e.getActionMasked()==MotionEvent.ACTION_UP) performClick();
        return true;
    }
    @Override public boolean performClick() { super.performClick();return true; }
}
