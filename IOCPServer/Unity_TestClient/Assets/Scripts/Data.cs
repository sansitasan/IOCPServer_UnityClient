using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public enum EMessageTypes
{
    None = 0,
    SignUp = 1,
    LogIn = 2,
}

[SerializeField]
public class SignUpData
{
    public EMessageTypes Type;
    public string ID;
    public string PW;

    public SignUpData(EMessageTypes type, string id, string pw)
    {
        Type = type;
        ID = id;
        PW = pw;
    }
}

[SerializeField]
public class ServMessage
{
    public string IP;
}
