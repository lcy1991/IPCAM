package com.liuuu.ipcamera;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.List;
import java.util.Timer;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.media.MediaRecorder;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.Toast;


public class IPCAM extends Activity implements OnClickListener {
	public native int  startRTSPServer();
	public native String helloWorld(String inputstr);
	static {
        System.loadLibrary("stage");
    }
	LocalServerSocket server;
	LocalSocket receiver, sender;
	String IPaddress;
	Button record, stop;
	Thread thread_loop;
	// ϵͳ��Ƶ�ļ�
	File viodFile;
	MediaRecorder mRecorder;
	// ��ʾ��Ƶ��SurfaceView
	SurfaceView sView;
	// ��¼�Ƿ����ڽ���¼��
	boolean isRecording = false;
	private static final String TAG = "IPCAM";
	Camera camera;
	@SuppressWarnings("deprecation")
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 
		setContentView(R.layout.main);
		record = (Button) findViewById(R.id.record);
		stop = (Button) findViewById(R.id.stop);
		sView = (SurfaceView) findViewById(R.id.dView);
		// stop��ť������
		stop.setEnabled(false);
		// ����Surface����Ҫά���Լ��Ļ�����
		sView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		// ���÷ֱ���
		//sView.getHolder().setFixedSize(480, 800);
	    RelativeLayout.LayoutParams params  =   
	    new RelativeLayout.LayoutParams(540, 540*720/480);  
	          
	    params.leftMargin = 0;  
	    params.topMargin  = 0;  
	            
	    sView.setLayoutParams(params);
		// ���ø������������Ļ�Զ��ر�
		sView.getHolder().setKeepScreenOn(true);
	 
		record.setOnClickListener(this);
		stop.setOnClickListener(this);
		
		startRTSPServer();//---------------------------------client--------------------------------------------------jni//
		IPaddress = getIPAddr();
		try {
			server = new LocalServerSocket("VideoCamera");	
			sender = server.accept();
			sender.setReceiveBufferSize(500000);
			sender.setSendBufferSize(500000);
			sender.getOutputStream().write(IPaddress.getBytes());
		} catch (IOException e) {
			Log.e(TAG, "outWriter error !!!!!!!!!!!!!!!!!");
			finish();
			return;
		}
		thread_daemon();
		

	}

	@Override
	 public boolean onCreateOptionsMenu(Menu menu) {
	  // Inflate the menu; this adds items to the action bar if it is present.
	 // getMenuInflater().inflate(R.main, menu);
	  return true;
	 }	
	
	@Override
	 public void onClick(View v) {
		switch (v.getId()) {
			case R.id.record:
				if (!Environment.getExternalStorageState().equals(
					Environment.MEDIA_MOUNTED)) {
					Toast.makeText(this, "SD�������ڣ���忨��", Toast.LENGTH_SHORT).show();
	    			return;
				}
				try {
				    // ����MediaPlayer����
				    mRecorder = new MediaRecorder();
				    mRecorder.reset();
				    camera = Camera.open();
				    getSupportSize(camera);
				    Camera.Parameters params = camera.getParameters();
				    //previewSize = params.getSupportedPreviewSizes();
				    params.setPreviewSize(720, 480);
				    //params.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
				    camera.setParameters(params);
				    camera.setDisplayOrientation(90);
				    //camera.setOneShotPreviewCallback(this);/////////////////////////////////////////////////
				    camera.unlock();	    
				    mRecorder.setCamera(camera);
				    // ��������¼����Ƶ����Ƶ�ļ�
				    viodFile = new File(Environment.getExternalStorageDirectory()
				      .getCanonicalFile() + "/myvideo.mp4");
			
				    if (!viodFile.exists())
				     viodFile.createNewFile();
				    // ���ô���˷�ɼ�����
				    //mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
				    // ���ô�����ͷ�ɼ�ͼ��
				    mRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
				    // ������Ƶ����Ƶ�������ʽ
				    mRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
				    // ������Ƶ�ı����ʽ��
				    //mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.DEFAULT);
				    // ����ͼ������ʽ
				    mRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
				    mRecorder.setVideoEncodingBitRate(500000);//500kbps
				    mRecorder.setOrientationHint(90);
				    mRecorder.setVideoSize(720, 480);
				    mRecorder.setVideoFrameRate(15);
				    mRecorder.setOutputFile(viodFile.getAbsolutePath());
				    // ָ��SurfaceView��Ԥ����Ƶ
				    mRecorder.setPreviewDisplay(sView.getHolder().getSurface());
				    mRecorder.prepare();
				    // ��ʼ¼��
				    mRecorder.start();
				    // ��record��ť������
				    record.setEnabled(false);
				    // ��stop��ť����
				    stop.setEnabled(true);
				    isRecording = true;		
				   } catch (Exception e) {
				    e.printStackTrace();
			    }
				   break;
		  case R.id.stop:
		   // �������¼��
		   if (isRecording) {
		    // ֹͣ¼��
		    mRecorder.stop();
		    // �ͷ���Դ
		    mRecorder.release();
		    mRecorder = null;
		    camera.stopPreview();
		    camera.release();
		    // ��record��ť����
		    record.setEnabled(true);
		    // ��stop��ť������
		    stop.setEnabled(false);
		   }
		   break;
		  default:
		   break;
	  }
	 }
	
	public void getSupportSize(Camera mCamera){
		//Get screen size
		int screenWidth;
		int screenHeight;
		WindowManager wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();        
        screenWidth = display.getWidth();
        screenHeight = display.getHeight();
        Log.d(TAG,"screenW:"+screenWidth+"*screenH:"+screenHeight);
		//find 
		Camera.Parameters parameters = mCamera.getParameters();
        List<Camera.Size> sizes = parameters.getSupportedPreviewSizes();
        int[] a = new int[sizes.size()];
        int[] b = new int[sizes.size()];
        for (int i = 0; i < sizes.size(); i++) {
            int supportH = sizes.get(i).height;
            int supportW = sizes.get(i).width;
            a[i] = Math.abs(supportW - screenHeight);
            b[i] = Math.abs(supportH - screenWidth);
            Log.d(TAG,"supportW:"+supportW+"supportH:"+supportH);
        }
        int minW=0,minA=a[0];
        for( int i=0; i<a.length; i++){
            if(a[i]<=minA){
                minW=i;
                minA=a[i];
            }
        }
        int minH=0,minB=b[0];
        for( int i=0; i<b.length; i++){
            if(b[i]<minB){
                minH=i;
                minB=b[i];
            }
        }
        Log.d(TAG,"result="+sizes.get(minW).width+"x"+sizes.get(minH).height);
	}
	   public void surfaceChanged(SurfaceHolder holder, int format, int width, int height){
	          //ʵ���Զ��Խ�
	             camera.autoFocus(new AutoFocusCallback() {
	                 @Override
	                 public void onAutoFocus(boolean success, Camera camera) {
	                  if(success){
	                	  Log.d(TAG,"autofocus success");
	                  }
	                 }

	             });
	    }	
	private String getIPAddr(){
		String IP;
	    //��ȡwifi����  
	    WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);  
	    //�ж�wifi�Ƿ���  
	    if (!wifiManager.isWifiEnabled()) {  
	    wifiManager.setWifiEnabled(true);    
	    }  
	    WifiInfo wifiInfo = wifiManager.getConnectionInfo();       
	    int ipAddress = wifiInfo.getIpAddress();   
	    IP = intToIp(ipAddress);
	    Log.d(TAG,"wifi IP address:"+IP);
	    return IP;
	}
	private String intToIp(int i) {       
	    
	    return (i & 0xFF ) + "." +       
	  ((i >> 8 ) & 0xFF) + "." +       
	  ((i >> 16 ) & 0xFF) + "." +       
	  ( i >> 24 & 0xFF) ;  
	} 

	private void thread_daemon() {
		Log.d(TAG, "thread_daemon start");
		(thread_loop = new Thread() {
			public void run() {
			byte[] bytes = new byte[10];
			int ret=0;
			InputStream fin = null;
			try {
				fin = sender.getInputStream();
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}			
			//initializeVideo();
			while(true)
			{	
				try {
					Thread.currentThread().sleep(500);
				} catch (InterruptedException e1) {
					e1.printStackTrace();
				}

				try {
					ret = fin.read(bytes, 0, 10);
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				if(ret > 0)
				{
					if(bytes[0] == 0x05)
					{
						Log.i(TAG,"Receive stop signal");
						stopRecording();
					}
					else if(bytes[0] == 0x06)
					{
						Log.i(TAG,"Receive start signal");
						startRecording(sender);		
					}
				}
			}

			}
		}).start();
	}
	
	public void startRecording(LocalSocket viodFile){
		try {
		    // ����MediaPlayer����
		    mRecorder = new MediaRecorder();
		    mRecorder.reset();
		    camera = Camera.open();
		    getSupportSize(camera);
		    Camera.Parameters params = camera.getParameters();
		    //previewSize = params.getSupportedPreviewSizes();
		    params.setPreviewSize(720, 480);
		    //params.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
		    camera.setParameters(params);
		    camera.setDisplayOrientation(90);
		    camera.unlock();
		    //camera.startPreview();////////////////////////////////////////////////////////////////////////////////////
		    mRecorder.setCamera(camera);

		    // ���ô���˷�ɼ�����
		    //mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
		    // ���ô�����ͷ�ɼ�ͼ��
		    mRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
		    // ������Ƶ����Ƶ�������ʽ
		    mRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
		    // ������Ƶ�ı����ʽ
		    //mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.DEFAULT);
		    // ����ͼ������ʽ
		    mRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
		    mRecorder.setVideoEncodingBitRate(500000);//500kbps
		    mRecorder.setVideoFrameRate(15);
		    mRecorder.setVideoSize(720, 480);
		    mRecorder.setOrientationHint(90);

		    mRecorder.setOutputFile(viodFile.getFileDescriptor());
		    // ָ��SurfaceView��Ԥ����Ƶ
		    mRecorder.setPreviewDisplay(sView.getHolder().getSurface());
		    mRecorder.prepare();
		    // ��ʼ¼��
		    mRecorder.start();
		    // ��record��ť������
		    record.setEnabled(false);
		    // ��stop��ť����
		    stop.setEnabled(true);
		    isRecording = true;
	
		   } catch (Exception e) {
		    e.printStackTrace();
	    }		
	}

	public void stopRecording(){
	   if (isRecording) {
		    // ֹͣ¼��
		    mRecorder.stop();
		    // �ͷ���Դ
		    mRecorder.release();
		    mRecorder = null;
		    camera.stopPreview();
		    camera.release();
		    // ��record��ť����
		    record.setEnabled(true);
		    // ��stop��ť������
		    stop.setEnabled(false);
		   }	
	}
	private int FindFrontCamera(){  
        int cameraCount = 0;  
        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();  
        cameraCount = Camera.getNumberOfCameras(); // get cameras number  
                
        for ( int camIdx = 0; camIdx < cameraCount;camIdx++ ) {  
            Camera.getCameraInfo( camIdx, cameraInfo ); // get camerainfo  
            if ( cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_FRONT ) {   
                // ��������ͷ�ķ�λ��Ŀǰ�ж���ֵ�����ֱ�ΪCAMERA_FACING_FRONTǰ�ú�CAMERA_FACING_BACK����  
               return camIdx;  
            }  
        }  
        return -1;  
    }  
   	
}