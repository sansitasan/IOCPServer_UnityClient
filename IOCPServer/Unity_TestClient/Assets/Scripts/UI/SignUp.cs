using Cysharp.Threading.Tasks;
using Google.Protobuf.Collections;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Text;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class SignUp : MonoBehaviour
{
    [SerializeField]
    private Button _signUpButton;
    [SerializeField]
    private Button _onOffButton;

    [SerializeField]
    private TMP_InputField _idField;
    [SerializeField]
    private TMP_InputField _pwField;

    private bool _bSend = false;

    private void Awake()
    {
        _signUpButton.onClick.AddListener(Sign);
        _onOffButton.onClick.AddListener(OffPanel);
    }

    private void Sign()
    {
        if (!_bSend)
        {
            UniTask.RunOnThreadPool(SendIdPWParrellel).Forget();
        }
    }

    private void OffPanel()
    {
        LoginScene.Instance.SetOnOff();
    }

    private void SendIdPWParrellel()
    {
        _bSend = true;
        //SignUpData data = new SignUpData(EMessageTypes.SignUp, _idField.text, _pwField.text);
        //string send = JsonUtility.ToJson(data);
        //byte[] buffer = Encoding.UTF8.GetBytes(send);
        //LoginScene.Instance.Sock.BeginSend(buffer, 0, buffer.Length, SocketFlags.None,
        //new AsyncCallback(SendCallback), send);
        SearchRequest s = new SearchRequest();
        s.Query = EMessageTypes.SignUp .ToString() + _idField.text + _pwField.text;
        s.PageNumber = 2;
        s.ResultsPerPage = s.Query.Length;
        var stream = new byte[s.CalculateSize()];
        var outStream = new Google.Protobuf.CodedOutputStream(stream);
        s.WriteTo(outStream);
        LoginScene.Instance.Sock.BeginSend(stream, 0, stream.Length, SocketFlags.None,
        new AsyncCallback(SendCallback), outStream);
    }

    private void SendCallback(IAsyncResult result)
    {
        Debug.Log("전송 완료");
        _bSend = false;
    }
}
