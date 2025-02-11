/*
 * Copyright (c) 2011, Sony Ericsson Mobile Communications AB.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sony Ericsson Mobile Communications AB nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package com.sonyericsson.TouchpadNAJava;

import ${YYAndroidPackageName}.RunnerActivity;

import android.view.Window;
import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;

public class TouchpadNAActivity extends NativeActivity {
    static int pos_x=0;
    static int pos_y=0;


    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i("yoyo", "ON CREATE!!!!!!!!!!!!!");
        RegisterThis();
        getWindow().takeSurface( null );
        RunnerActivity.CurrentActivity.setupView();
    }

    public boolean OnNativeMotion( int action, int x, int y, int source, int device_id ) {
    	Log.i("yoyo", "Received native motion event! (" + x + ", " + y + ", " + source + ", " + device_id + ")");
        pos_x=x;
        pos_y=y;
    	return true;
    }

    public static double GetPosX()
    {
        return pos_x;
    }

    public static double GetPosY()
    {
        return pos_y;
    }

    native int RegisterThis();

    public static native double TestLink();
    public static native double StartInputQueue();

    static {
        System.loadLibrary("touchpadndkjava");
    }
}