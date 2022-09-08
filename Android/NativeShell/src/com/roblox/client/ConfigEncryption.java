package com.roblox.client;

import java.io.IOException;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.PBEParameterSpec;

import android.util.Base64;

public class ConfigEncryption {
	
	// Use Marsaglia's xorshf/xorshf96 random number generator to generate the password.
    //period 2^96-1
	private static long x=0xd5be4b59, y=0xbfb998ee, z=0x427cca47;
	private static long xors()
	{
		long t;
	
		x ^= x << 16;
		x ^= x >> 5;
		x ^= x << 1;
	
		t = x;
		x = y;
		y = z;
		z = t ^ x ^ y;
		return z;
	}

	private static byte[] xorssal()
	{
		int t=13;
		while( --t > 0 )
		{
			xors();
		}
		
		final int sz = 8;
		byte[] s = new byte[sz];		
		for( int i=0; i < sz; ++i )
		{
			s[i] = (byte)xors();
		}
		return s;
	}
	
	private static char[] xorspw()
	{
		String s = base64Encode( xorssal() );
		return s.toCharArray();
	}
	
    private static final char[] pw = xorspw();
    private static final byte[] sal = xorssal();

    private static byte[] base64Decode(String property) throws IOException {
    	return Base64.decode( property, Base64.NO_WRAP|Base64.NO_PADDING );
    }

    private static String base64Encode(byte[] bytes) {
    	return Base64.encodeToString( bytes, Base64.NO_WRAP|Base64.NO_PADDING );
    }
    
    public static String encrypt(String property)
    {
    	String result = property;
    	try
    	{
	        SecretKeyFactory keyFactory = SecretKeyFactory.getInstance("PBEWithMD5AndDES");
	        SecretKey key = keyFactory.generateSecret(new PBEKeySpec(pw));
	        Cipher pbeCipher = Cipher.getInstance("PBEWithMD5AndDES");
	        pbeCipher.init(Cipher.ENCRYPT_MODE, key, new PBEParameterSpec(sal, 20));
	        result = base64Encode(pbeCipher.doFinal(property.getBytes("UTF-8")));
    	}
    	catch( Exception ex )
    	{
    	}
		return result;
    }

    public static String decrypt(String property) {
    	String result = property;
    	try
    	{
	        SecretKeyFactory keyFactory = SecretKeyFactory.getInstance("PBEWithMD5AndDES");
	        SecretKey key = keyFactory.generateSecret(new PBEKeySpec(pw));
	        Cipher pbeCipher = Cipher.getInstance("PBEWithMD5AndDES");
	        pbeCipher.init(Cipher.DECRYPT_MODE, key, new PBEParameterSpec(sal, 20));
	        result = new String(pbeCipher.doFinal(base64Decode(property)), "UTF-8");
    	}
    	catch( Exception ex )
    	{
    	}
		return result;
    }
}
