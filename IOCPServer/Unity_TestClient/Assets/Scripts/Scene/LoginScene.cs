using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System;
using Cysharp.Threading.Tasks;
using System.Text;

public class LoginScene : MonoBehaviour
{
    [SerializeField]
    private Canvas _signUp;
    [SerializeField]
    private Canvas _logIn;

    public static LoginScene Instance;

    public const string ServIPstring = "222.101.210.36";
    public const long ServIPint = 3731214884;
    public Socket Sock { get; private set; }

    private void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
            Init();
        }
        else
            Destroy(gameObject);
    }

    private void Init()
    {
        Sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        Sock.BeginConnect(ServIPstring, 4444, new AsyncCallback(ConnectCallback), Sock);
    }

    private void AcceptMessage()
    {
        WaitReceive().Forget();
    }

    private async UniTask WaitReceive()
    {
        byte[] buffer = new byte[1024];
        while (Sock.Connected)
        {
            buffer.Initialize();
            var check = Sock.BeginReceive(buffer, 0, buffer.Length, SocketFlags.None, new AsyncCallback(GetMessage), buffer);
            await UniTask.WaitUntil(() => check.IsCompleted);
        }
    }

    private void GetMessage(IAsyncResult result)
    {
        if (result.AsyncState is byte[])
        {
            string str = Encoding.Default.GetString(result.AsyncState as byte[]);
            Debug.Log(str);
        }
    }

    private void ConnectCallback(IAsyncResult ar)
    {
        try
        {
            Debug.Log("연결 성공");
            UniTask.RunOnThreadPool(AcceptMessage).Forget();
            Sock.BeginSend(Encoding.UTF8.GetBytes("123"), 0, 3, SocketFlags.None, new AsyncCallback(_ => Debug.Log("가짜")), null);
        }

        catch (Exception ex)
        {
            Debug.LogException(ex);
            Application.Quit();
        }
    }

    public void SetOnOff()
    {
        if (_signUp.gameObject.activeSelf)
        {
            _signUp.gameObject.SetActive(false);
            _logIn.gameObject.SetActive(true);
        }

        else
        {
            _signUp.gameObject.SetActive(true);
            _logIn.gameObject.SetActive(false);
        }
    }

    private void OnDestroy()
    {
        Sock.Close();
    }
}
