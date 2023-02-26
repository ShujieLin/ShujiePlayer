package com.shujie.shujieplayer;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.shujie.shujieplayer.databinding.ActivityMainBinding;
import com.shujie.shujieplayer.player.Player;

import java.io.File;
import java.util.Arrays;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;

    private Player player;
    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        requestPermisson();

        player = new Player();
        player.setDataSource(new File(Environment.getExternalStorageDirectory() + File.separator + "demo.mp4").getAbsolutePath());
        Log.d(TAG, "onCreate: Environment.getExternalStorageDirectory() = " + Environment.getExternalStorageDirectory());

        player.setOnpreparedListener(new Player.OnPreparedListener() {
            @Override
            public void onPrepared() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG, "run: 准备成功，即将开始播放");
                        Toast.makeText(MainActivity.this, "准备成功，即将开始播放", Toast.LENGTH_SHORT).show();
                    }
                });
                player.start(); // 调用 C++ 开始播放
            }
        });

        player.setOnErrorListener(new Player.OnErrorListener() {
            @Override
            public void onError(String errorCode) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        // Toast.makeText(MainActivity.this, "出错了，错误详情是:" + errorInfo, Toast.LENGTH_SHORT).show();
                        binding.tvState.setTextColor(Color.RED); // 红色
                        binding.tvState.setText("哎呀,错误啦，错误:" + errorCode);
                    }
                });
            }
        });
    }

    @Override // ActivityThread.java Handler
    protected void onResume() { // 我们的准备工作：触发
        super.onResume();
        player.prepare();
    }

    @Override
    protected void onStop() {
        super.onStop();
        player.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
    }

    private void requestPermisson() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                    0);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Log.d(TAG, "onRequestPermissionsResult() called with: requestCode = [" + requestCode + "], permissions = [" + Arrays.toString(permissions) + "], grantResults = [" + Arrays.toString(grantResults) + "]");
        if (requestCode == 0) {
            boolean isGranded = true;
            for (int i :
                    grantResults) {
                if (i == PackageManager.PERMISSION_DENIED){
                    isGranded = false;
                }
            }
            Log.d(TAG, "onRequestPermissionsResult: " + isGranded);
        }
    }
}