using System.Collections;
using System.Collections.Generic;
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Cysharp.Threading.Tasks;

public class Login : MonoBehaviour
{
    [SerializeField]
    private Button _loginButton;
    [SerializeField]
    private Button _onOffButton;

    [SerializeField]
    private TMP_InputField _idField;
    [SerializeField]
    private TMP_InputField _pwField;

    private bool _bSend = false;

    public void LogIn()
    {
        if (!_bSend && LoginScene.Instance.Sock.Connected)
        {
            UniTask.RunOnThreadPool(SendIdPWParrellel).Forget();
        }
    }

    private void SendIdPWParrellel()
    {
        _bSend = true;
        StringBuilder sb = new StringBuilder();
        sb.Append((int)EMessageTypes.LogIn);
        sb.Append('/');
        sb.Append(_idField.text);
        sb.Append('/');
        sb.Append(_pwField.text);
        byte[] buffer = Encoding.UTF8.GetBytes(sb.ToString());
        LoginScene.Instance.Sock.BeginSend(buffer, 0, buffer.Length, SocketFlags.None,
            new AsyncCallback(SendCallback), sb.ToString());
    }

    private void SendCallback(IAsyncResult result)
    {
        Debug.Log("전송 완료");
        _bSend = false;
    }
}
