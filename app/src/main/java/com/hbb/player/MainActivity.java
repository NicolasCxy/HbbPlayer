package com.hbb.player;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.hbb.ffmpeg.impl.PlayerStateChangeListener;
import com.hbb.ffmpeg.opengl.HBBGLSurfaceView;
import com.hbb.ffmpeg.palyer.HbbPlayer;
import com.hbb.player.utils.DisplayUtil;

import java.io.File;

public class MainActivity extends AppCompatActivity  {
    private static final String TAG = "MainActivity";

    private HbbPlayer mHbbPlayer;

    File file = new File(Environment.getExternalStorageDirectory(), "newTwo.mp4");
    File file2 = new File(Environment.getExternalStorageDirectory(), "JingQi.mp4");
    private HBBGLSurfaceView mHbbGlView;
    private SeekBar mSeekbar;
    private TextView mTvTime;
    private boolean seek = false;

    private Handler mHandler = new Handler(Looper.myLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        checkPermission();

        mHbbGlView = (HBBGLSurfaceView) findViewById(R.id.hbbGlView);
        mSeekbar = (SeekBar) findViewById(R.id.seekbar);
        mTvTime = (TextView) findViewById(R.id.tv_time);

        mHbbPlayer = new HbbPlayer();
        mHbbPlayer.setSurfaceView(mHbbGlView);
        mHbbPlayer.setChangeListener(playerStateChangeListener);
        mSeekbar.setOnSeekBarChangeListener(seekBarChangeListener);


    }

    public boolean checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
            }, 1);

        }
        return false;
    }

    public void play(View view) {
        if (null != mHbbPlayer) {
            mHbbPlayer.prepare(file.getAbsolutePath());
        }
    }



    int position;
    SeekBar.OnSeekBarChangeListener seekBarChangeListener = new SeekBar.OnSeekBarChangeListener() {

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            position = progress * mHbbPlayer.getDuration() / 100;

        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            seek = true;
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
//            Toast.makeText(MainActivity.this, "seek：" + position, Toast.LENGTH_SHORT).show();
            mHbbPlayer.seek(position);
            seek = false;
        }
    };

    public void stop(View view) {
        mHbbPlayer.stop();
    }

    public void pause(View view) {
        mHbbPlayer.pause();
    }

    public void resume(View view) {
        mHbbPlayer.resume();
    }


    public void reUIState() {
        mSeekbar.setProgress(0);
        mTvTime.setText("00:00/00:00");
    }

    public void speed1(View view) {
        mHbbPlayer.speed(1.5f);
    }

    public void speed2(View view) {
        mHbbPlayer.speed(2.0f);
    }

    public void next(View view) {
        mHbbPlayer.stop();
        mHbbPlayer.prepare(file2.getAbsolutePath());
    }


    PlayerStateChangeListener playerStateChangeListener = new PlayerStateChangeListener() {
        @Override
        public void onBeReady() {
            if (null != mHbbPlayer) {
                Log.i(TAG, "mHbbPlayer.start");
                mHbbPlayer.start();
            }
        }

        @Override
        public void onCurrentTime(int currentTime, int totalTime) {
            if (!seek && totalTime > 0) {
                mHandler.post(() -> {
                    mSeekbar.setProgress(currentTime * 100 / totalTime);
                    mTvTime.setText(DisplayUtil.secdsToDateFormat(currentTime)
                            + "/" + DisplayUtil.secdsToDateFormat(totalTime));
                });

            }
        }

        @Override
        public void onComplete() {
            runOnUiThread(() -> {
                reUIState();
                Toast.makeText(MainActivity.this, "即将开始播放下一个", Toast.LENGTH_SHORT).show();
            });

            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    //开始播放一个
                    if (null != mHbbPlayer) {
                        mHbbPlayer.prepare(file2.getAbsolutePath());
                    }
                }
            },2000);
        }

        @Override
        public void onStop() {
            runOnUiThread(() -> {
                reUIState();
            });
        }
    };
}