using System;
using System.Security.Cryptography;
using System.Text;

/// <summary>
/// SharedSecurityNotary does the actual signing of scripts, but does not load the private key
/// </summary>
public class SharedSecurityNotary
{
    public SharedSecurityNotary()
    {
    }

    public static string CreateSignature(string message, byte[] blob)
    {
        var cspParam = new CspParameters();
        cspParam.Flags = CspProviderFlags.UseMachineKeyStore;

        byte[] asciiMessage = ASCIIEncoding.ASCII.GetBytes(message);
        byte[] signature;

        using (var rsaCryptoServiceProvider = new RSACryptoServiceProvider(cspParam))
        {
            rsaCryptoServiceProvider.ImportCspBlob(blob);
            using (var sha1CryptoServiceProvider = new SHA1CryptoServiceProvider())
            {
                signature = rsaCryptoServiceProvider.SignData(asciiMessage, sha1CryptoServiceProvider);
            }
        }

        string encodedSignature = Convert.ToBase64String(signature);
        return encodedSignature;
    }
    public static string SignScript(string script, byte[] blob)
    {
        string signature = CreateSignature(script, blob);
        string signedScript = string.Format("%{0}%{1}", signature, script);
        return signedScript;
    }
}