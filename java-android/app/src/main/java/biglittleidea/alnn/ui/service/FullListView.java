// https://newbedev.com/android-wrap-content-is-not-working-with-listview
package biglittleidea.alnn.ui.service;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ListView;

public class FullListView extends ListView {

    public FullListView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public FullListView(Context context) {
        super(context);
    }

    public FullListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int expandSpec = MeasureSpec.makeMeasureSpec(Integer.MAX_VALUE >> 2,
                MeasureSpec.AT_MOST);
        super.onMeasure(widthMeasureSpec, expandSpec);
    }

}
