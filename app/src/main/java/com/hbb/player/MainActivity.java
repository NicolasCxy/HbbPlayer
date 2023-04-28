package com.hbb.player;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;

import com.hbb.ffmpeg.impl.PlayerStateChangeListener;
import com.hbb.ffmpeg.palyer.HbbPlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity implements PlayerStateChangeListener {

    private HbbPlayer mHbbPlayer;

    File file = new File(Environment.getExternalStorageDirectory(),"JingQi.mp4");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        checkPermission();

        mHbbPlayer = new HbbPlayer();
        mHbbPlayer.setChangeListener(this);
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
        if(null != mHbbPlayer){
            mHbbPlayer.prepare(file.getAbsolutePath());
        }
    }

    @Override
    public void onBeReady() {
        if(null!= mHbbPlayer){
            mHbbPlayer.start();
        }
    }
}