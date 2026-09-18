package hsh.mplab.soaapinterface;

import java.io.IOException;
import java.net.Socket;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;



public class SOAAPClient {


    // =========================================================================
    // SOAAP UI Version
    // =========================================================================
    public enum SOAAP_UI_VERSION_NR
    {
        None,
        SOAAP_UI_1_Prof_Patzke_Konfiguration
    }



    // =========================================================================
    // Datentypen
    // =========================================================================

    public enum Meas2Midi
    {
        Nothing,        // keine Zielzuweisung
        NoteType,       // Zuweisung Notentyp (Laeng)
        NoteVal,        // Zuweisung NOtenwert (Hoehe)
        NoteVel         // Zuweisung Anschagstaeke
    }

    public enum MeasMap
    {
        OneToOne,       // Direkte Weruebertragung
        BiLinear       // Linear auf Lienar
    }

    public enum MidiOpMode
    {
        momIdel,
        momSequence,
        momRunDealt
    }

    // neue Datensatz

    public interface SOAAP_UI_1_DATENSATZ_KONFIGURATION
    {
        static final int nrInt = 8;
        static final int nrFloat = 9;
        static final int nrByte = 6;
        static final int nrChannel = 16;
    }

    public class SoaapUiDatensatz
    {
        int             channel;

        float           rollwinkelAreaOffset;
        float           rollwinkelAreaMin;
        float           rollwinkelAreaMax;

        float           pitchwinkelAreaOffset;
        float           pitchwinkelAreaMin;
        float           pitchwinkelAreaMax;

        float           yawwinkelAreaOffset;
        float           yawwinkelAreaMin;
        float           yawwinkelAreaMax;

        byte            midiAreaNoteTypeHigh;
        byte            midiAreaNoteTypeLow;
        byte            midiAreaNoteValHigh;
        byte            midiAreaNoteValLow;
        byte            midiAreaNoteVelHigh;
        byte            midiAreaNoteVelLow;

        Meas2Midi       aimRoll;
        Meas2Midi       aimPitch;
        Meas2Midi       aimYaw;

        MeasMap         mapRoll;
        MeasMap         mapPitch;
        MeasMap         mapYaw;

        MidiOpMode      midiOpMode;

        public void init()
        {
            channel = 0;

            rollwinkelAreaOffset = 50.0F;
            rollwinkelAreaMax = 90.0F;
            rollwinkelAreaMin = 10.0F;
            pitchwinkelAreaOffset = 50.0F;
            pitchwinkelAreaMax = 90.0F;
            pitchwinkelAreaMin = 10.0F;
            yawwinkelAreaOffset = 50.0F;
            yawwinkelAreaMax = 90.0F;
            yawwinkelAreaMin = 10.0F;

            midiAreaNoteTypeHigh = (byte) 90;
            midiAreaNoteTypeLow = (byte) 10;
            midiAreaNoteValHigh = (byte) 90;
            midiAreaNoteValLow = (byte) 10;
            midiAreaNoteVelHigh = (byte) 90;
            midiAreaNoteVelLow = (byte) 10;

            aimRoll = Meas2Midi.Nothing;
            aimPitch = Meas2Midi.Nothing;
            aimYaw = Meas2Midi.Nothing;

            mapRoll = MeasMap.BiLinear;
            mapPitch = MeasMap.BiLinear;
            mapYaw = MeasMap.BiLinear;

            midiOpMode = MidiOpMode.momSequence;
        }
    }


    public SoaapUiDatensatz[] soaapUiDaten;

    public String        defaultIP = "192.168.4.1";
    public int           defaultPort = 4210;

    // Lokale Variablen für die interne Verarbeitung
    //

    private int                 errorCode;
    private String              errorMsg;
    private String              resultMsg;

    private Socket              socket; // Instanz von Socket
    private String              APConnectIP;
    private int                 APConnectPort;

    private InputStream         is; // input Stream
    private InputStreamReader   isr ; // input Stream auslesen
    private BufferedReader      br ; // read buffer
    String                      response; // data vom Server

    private OutputStream        outputStream;
    private boolean             isConnected;
    private boolean             fin;

    // Pdu Array mit drei Datentypen - int, float , byte

    public float[]             PdufloatArray;             // PduRaumwinkelAreaArray enthält die 9 Parameter,
                                                                    // Dies sind die Parameter, die für die räumliche Winkelzuordnung mapping zu den Midi-Daten benötigt werden (offset, min, max)
                                                                    //       idx              Name des Paramerters
                                                                    //        0                 Roll offset
                                                                    //        1                 Roll min
                                                                    //        2                 Roll max
                                                                    //        3                 Pitch offset
                                                                    //        4                 Pitch min
                                                                    //        5                 Pitch max
                                                                    //        6                 Yaw offset
                                                                    //        7                 Yaw min
                                                                    //        8                 Yaw max
    public int[]               PduintArray;                        // PduAimsArray enthält die 8 Parameter
                                                                    // Ein Parameter ist Channel
                                                                    //        idx-0             Channel
                                                                    // Drei Parameter weisen jeder der drei Raumecken Kontrollziele (bmp, value, velocity) zu,
                                                                    //        idx               Raumwinkel     |        value           Kontrollziel                     
                                                                    //         1                   Roll        |          0                 Nothing
                                                                    //         2                  Pitch        |          1                 bpm         (NoteType)
                                                                    //         3                   Yaw         |          2                 value       (NoteVal)
                                                                    //                                         |          3                 velocity    (NoteVel)
                                                                    // Drei Parameter sind die Mapping-Methode
                                                                    //        idx               Raumwinkel     |        value            mapping-mode
                                                                    //         4                   Roll        |          0                OneToOne
                                                                    //         5                  Pitch        |          1                BiLinear
                                                                    //         6                   Yaw         |
                                                                    // Der letzte Parameter ist OpMode
                                                                    //        idx-7              OpMode        |        value                mode
                                                                    //                                                    0                 momIdel,
                                                                    //                                                    1                momSequence,
                                                                    //                                                    2                momRunDealt

    public byte[]              PdubyteArray;                       // PduMidiAreaArray enthält die 6 Parameter,
                                                                    // Dies sind die Parameter, die für die räumliche Winkelzuordnung mapping zu den Midi-Daten benötigt werden (high, low)
                                                                    //       idx              Name des Paramerters
                                                                    //        0                   NoteType high
                                                                    //        1                   NoteType low
                                                                    //        2                   NoteVal high
                                                                    //        3                   NoteVal low
                                                                    //        4                   NoteVel high
                                                                    //        5                   NoteVel low

    // -------------------------------------------------------------------------
    // region Konstruktoren
    // -------------------------------------------------------------------------

    public SOAAPClient() {
        init(defaultIP, defaultPort, Speed.high, 2, 2, 2);
    }

    public SOAAPClient(int nrInt, int nrFloat, int nrByte) {
        init(defaultIP, defaultPort, Speed.high, nrInt, nrFloat, nrByte);
    }

    public SOAAPClient(String ip, int port) {
        init(ip, port, Speed.high, 2, 2, 2);
    }

    public SOAAPClient(String ip, int port, Speed inSpeed)
    {
        init(ip, port, inSpeed, 2, 2, 2);
    }

    public SOAAPClient(SOAAP_UI_VERSION_NR version)
    {
        switch (version)
        {
            case None:
                break;

            case SOAAP_UI_1_Prof_Patzke_Konfiguration:
                init(defaultIP,
                     defaultPort,
                     Speed.high,
                     SOAAP_UI_1_DATENSATZ_KONFIGURATION.nrInt,
                     SOAAP_UI_1_DATENSATZ_KONFIGURATION.nrFloat,
                     SOAAP_UI_1_DATENSATZ_KONFIGURATION.nrByte);

                soaapUiDaten = new SoaapUiDatensatz[SOAAP_UI_1_DATENSATZ_KONFIGURATION.nrChannel];

                for(int i = 0; i < SOAAP_UI_1_DATENSATZ_KONFIGURATION.nrChannel; i++)
                {
                    soaapUiDaten[i] = new SoaapUiDatensatz();
                }

                for(int i = 0; i < 16; i++)
                {
                    soaapUiDaten[i].init();
                }

                break;

            default:
                break;
        }
    }

    // -------------------------------------------------------------------------
    // region Initialisierungen
    // -------------------------------------------------------------------------

    void init(String ip, int port, Speed inSpeed, int nrInt, int nrFloat, int nrByte)
    {
        errorCode              = 0;
        APConnectIP            = ip;
        APConnectPort          = port;
        speed                  = inSpeed;

        isConnected            = false;

        if(nrInt>0)
            PduintArray            = new int[nrInt];
        if(nrFloat>0)
            PdufloatArray          = new float[nrFloat];
        if(nrByte>0)
            PdubyteArray           = new byte[nrByte];

        if(errorCode != 0)
        {
            return;
        }
    }

    // -------------------------------------------------------------------------
    // region Einfache Hilfsfunktionen  ( Byte -> String )
    // -------------------------------------------------------------------------

    String getByteString(byte[] byteArray)
    {
        String retStr = "";

        if(byteArray == null)
        {
            retStr = "00;00;00;00;00;00";
            return(retStr);
        }

        for(int i = 0; i < byteArray.length; i++)
        {
            retStr += String.format("%02X;", byteArray[i]);
        }

        return(retStr.substring(0, retStr.length()-1));
    }

    // -------------------------------------------------------------------------
    // region Anwenderfunktionen  -  Send
    // -------------------------------------------------------------------------

    public enum Speed
    {
        normal,
        high,
        low,
        nrOfSpeeds
    }

    public boolean enabled = true;
    public Speed speed;


    public enum RunStatus
    {
        Connect,
        Wait,
        CreateCheckMsg,
        SendCheckConnect,
        ConfirmCheckMsg,
        Close,
        Error,
        NrOfStates
    }

    public RunStatus runStatus = RunStatus.Connect;
    //public RunStatus runError  = RunStatus.NrOfStates;

    String  runErrorMsg;
    int     delayCounter;
    int     speedCounter = 8;
    byte[]  sendBuffer;

    int     SocketFailedCounter = 5;

    public void run(int secFactor, int delay)
    {
        int speedLimit;

        if(!enabled)
        {
            if(runStatus == RunStatus.Wait)
                return;
        }

        if(delay > 0)
        {
            delayCounter++;
            if(delayCounter < delay) return;
        }

        switch (runStatus)
        {
            case Connect:
                try
                {
                    // Instanz von Socket
                    socket = new Socket(APConnectIP, APConnectPort);
                    // Ob der Client mit dem Server erfolgreich verbunden ist
                    System.out.println(socket.isConnected() + "Socket OK");
                    isConnected = true;
                    runStatus = RunStatus.Wait;
                }
                catch (IOException exc)
                {
                    runErrorMsg = exc.getMessage();
                    errorMsg = "run.connect Error: " + runErrorMsg;
                    errorCode = 1;
                    //runError = runStatus;

                    if(SocketFailedCounter >= 0)
                    {
                        runStatus = RunStatus.Wait;
                        SocketFailedCounter--;
                    }
                    else
                    {
                        runStatus = RunStatus.Error;
                    }

                    System.out.println(errorMsg);
                    isConnected = false;
                }

            case Wait:
                speedCounter++;
                if(speed == Speed.high)
                    speedLimit = secFactor/10;
                else if(speed == Speed.low)
                    speedLimit = secFactor * 10;
                else
                    speedLimit = secFactor;
                if(speedCounter >= speedLimit)
                {
                    speedCounter  = 8;
                    runStatus     = RunStatus.CreateCheckMsg;
                }
                //System.out.println("WAIT" + speedCounter);
                break;

            case SendCheckConnect:
                // send Msg mit outputStream = socket.getOutputStream();
                runStatus = RunStatus.ConfirmCheckMsg;
                break;

            case ConfirmCheckMsg:
                runStatus = RunStatus.Wait;
                break;

            case Close:
                try
                {
                    outputStream.close();
                    socket.close();
                    isConnected = false;
                }
                catch ( IOException e)
                {
                    e.printStackTrace();
                }

            case Error:
                runStatus = RunStatus.Connect;
                isConnected = false;
                break;

            default:
                runStatus = RunStatus.Wait;
                break;
        }
    }


    // -------------------------------------------------------------------------
    // Anwendungsfunktionen zum Datensatz
    // -------------------------------------------------------------------------

    public void run(int secFactor)
    {
        run(secFactor, 0);
    }

    // ----------------SET----------------
    // int Array
    public void setChannel(int chn)
    {   if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].channel = chn;
    }

    public void setAimRoll(int chn, Meas2Midi aim)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        if(soaapUiDaten == null) return;
        soaapUiDaten[chn].aimRoll = aim;
    }

    public void setAimPitch(int chn, Meas2Midi aim)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        if(soaapUiDaten == null) return;
        soaapUiDaten[chn].aimPitch = aim;
    }

    public void setAimYaw(int chn, Meas2Midi aim)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        if(soaapUiDaten == null) return;
        soaapUiDaten[chn].aimYaw = aim;
    }

    public void setMappingRoll(int chn, MeasMap map)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        if(soaapUiDaten == null) return;
        soaapUiDaten[chn].mapRoll = map;
    }

    public void setMappingPitch(int chn, MeasMap map)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        if(soaapUiDaten == null) return;
        soaapUiDaten[chn].mapPitch = map;
    }

    public void setMappingYaw(int chn, MeasMap map)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        if(soaapUiDaten == null) return;
        soaapUiDaten[chn].mapYaw = map;
    }

    public void setOpMode(int chn, MidiOpMode opmode)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        if(soaapUiDaten == null) return;
        soaapUiDaten[chn].midiOpMode = opmode;
    }

    // float Array

    public void setRollAreaOffset(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].rollwinkelAreaOffset = value;
    }
    public void setRollAreaMin(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].rollwinkelAreaMin = value;
    }
    public void setRollAreaMax(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].rollwinkelAreaMax = value;
    }

    public void setPitchAreaOffset(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].pitchwinkelAreaOffset = value;
    }
    public void setPitchAreaMin(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].pitchwinkelAreaMin = value;
    }
    public void setPitchAreaMax(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].pitchwinkelAreaMax = value;
    }

    public void setYawAreaOffset(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].yawwinkelAreaOffset = value;
    }
    public void setYawAreaMin(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].yawwinkelAreaMin = value;
    }
    public void setYawAreaMax(int chn, float value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].yawwinkelAreaMax = value;
    }

    // byte Array

    public void setMidiNoteTypeAreaHigh(int chn, byte value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].midiAreaNoteTypeHigh = value;
    }
    public void setMidiNoteTypeAreaLow(int chn, byte value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].midiAreaNoteTypeLow = value;
    }

    public void setMidiNoteValAreaHigh(int chn, byte value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].midiAreaNoteValHigh = value;
    }
    public void setMidiNoteValAreaLow(int chn, byte value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].midiAreaNoteValLow = value;
    }

    public void setMidiNoteVelAreaHigh(int chn, byte value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].midiAreaNoteVelHigh = value;
    }
    public void setMidiNoteVelAreaLow(int chn, byte value)
    {
        if(chn < 0) return;
        if(chn > 16) return;
        soaapUiDaten[chn].midiAreaNoteVelLow = value;
    }

    // ----------------GET----------------

    public int getChannel(int chn)
    {
        return soaapUiDaten[chn].channel;
    }
    public Meas2Midi getAimRoll(int chn)
    {
        return soaapUiDaten[chn].aimRoll;
    }
    public Meas2Midi getAimPitch(int chn)
    {
        return soaapUiDaten[chn].aimPitch;
    }
    public Meas2Midi getAimYaw(int chn)
    {
        return soaapUiDaten[chn].aimYaw;
    }
    public MeasMap getMappingRoll(int chn)
    {
        return soaapUiDaten[chn].mapRoll;
    }
    public MeasMap getMappingPitch(int chn)
    {
        return soaapUiDaten[chn].mapPitch;
    }
    public MeasMap getMappingYaw(int chn)
    {
        return soaapUiDaten[chn].mapYaw;
    }
    public MidiOpMode getOpMode(int chn)
    {
        return soaapUiDaten[chn].midiOpMode;
    }


    public float getRollAreaOffset(int chn)
    {
        return soaapUiDaten[chn].rollwinkelAreaOffset;
    }
    public float getRollAreaMin(int chn)
    {
        return soaapUiDaten[chn].rollwinkelAreaMin;
    }
    public float getRollAreaMax(int chn)
    {
        return soaapUiDaten[chn].rollwinkelAreaMax;
    }

    public float getPitchAreaOffset(int chn)
    {
        return soaapUiDaten[chn].pitchwinkelAreaOffset;
    }
    public float getPitchAreaMin(int chn)
    {
        return soaapUiDaten[chn].pitchwinkelAreaMin;
    }
    public float getPitchAreaMax(int chn)
    {
        return soaapUiDaten[chn].pitchwinkelAreaMax;
    }

    public float getYawAreaOffset(int chn)
    {
        return soaapUiDaten[chn].yawwinkelAreaOffset;
    }
    public float getYawAreaMin(int chn)
    {
        return soaapUiDaten[chn].yawwinkelAreaMin;
    }
    public float getYawAreaMax(int chn)
    {
        return soaapUiDaten[chn].yawwinkelAreaMax;
    }


    public byte getMidiNoteTypeAreaHigh(int chn)
    {
        return soaapUiDaten[chn].midiAreaNoteTypeHigh;
    }
    public byte getMidiNoteTypeAreaLow(int chn)
    {
        return soaapUiDaten[chn].midiAreaNoteTypeLow;
    }

    public byte getMidiNoteValAreaHigh(int chn)
    {
        return soaapUiDaten[chn].midiAreaNoteValHigh;
    }
    public byte getMidiNoteValAreaLow(int chn)
    {
        return soaapUiDaten[chn].midiAreaNoteValLow;
    }

    public byte getMidiNoteVelAreaHigh(int chn)
    {
        return soaapUiDaten[chn].midiAreaNoteVelHigh;
    }
    public byte getMidiNoteVelAreaLow(int chn)
    {
        return soaapUiDaten[chn].midiAreaNoteVelLow;
    }

    // -------------------------------------------------------------
    // set Datensatz -> Pdu Daten (Array)

    public void setPduDatensatz(int chn)
    {
        if(chn < 0) return;
        if(chn > 16) return;

        setPduintArray(chn);
        setPdufloatArray(chn);
        setPdubyteArray(chn);
    }

    public void setPduintArray(int chn)
    {
        if(chn < 0) return;
        if(chn > 16) return;

        PduintArray[0] = chn;

        switch (soaapUiDaten[chn].aimRoll)
        {
            case Nothing:
                PduintArray[1] = 0;
                break;

            case NoteType:
                PduintArray[1] = 1;
                break;

            case NoteVal:
                PduintArray[1] = 2;
                break;

            case NoteVel:
                PduintArray[1] = 3;
                break;

            default:
                break;
        }

        switch (soaapUiDaten[chn].aimPitch)
        {
            case Nothing:
                PduintArray[2] = 0;
                break;

            case NoteType:
                PduintArray[2] = 1;
                break;

            case NoteVal:
                PduintArray[2] = 2;
                break;

            case NoteVel:
                PduintArray[2] = 3;
                break;

            default:
                break;
        }

        switch (soaapUiDaten[chn].aimYaw)
        {
            case Nothing:
                PduintArray[3] = 0;
                break;

            case NoteType:
                PduintArray[3] = 1;
                break;

            case NoteVal:
                PduintArray[3] = 2;
                break;

            case NoteVel:
                PduintArray[3] = 3;
                break;

            default:
                break;
        }

        switch (soaapUiDaten[chn].mapRoll)
        {
            case OneToOne:
                PduintArray[4] = 0;
                break;

            case BiLinear:
                PduintArray[4] = 1;
                break;

            default:
                break;
        }

        switch (soaapUiDaten[chn].mapPitch)
        {
            case OneToOne:
                PduintArray[5] = 0;
                break;

            case BiLinear:
                PduintArray[5] = 1;
                break;

            default:
                break;
        }

        switch (soaapUiDaten[chn].mapYaw)
        {
            case OneToOne:
                PduintArray[6] = 0;
                break;

            case BiLinear:
                PduintArray[6] = 1;
                break;

            default:
                break;
        }

        switch (soaapUiDaten[chn].midiOpMode)
        {
            case momIdel:
                PduintArray[7] = 0;
                break;

            case momSequence:
                PduintArray[7] = 1;
                break;

            case momRunDealt:
                PduintArray[7] = 2;
                break;

            default:
                break;
        }

    }

    public void setPdufloatArray(int chn)
    {
        if(chn < 0) return;
        if(chn > 16) return;

        PdufloatArray[0] = soaapUiDaten[chn].rollwinkelAreaOffset;
        PdufloatArray[1] = soaapUiDaten[chn].rollwinkelAreaMin;
        PdufloatArray[2] = soaapUiDaten[chn].rollwinkelAreaMax;

        PdufloatArray[3] = soaapUiDaten[chn].pitchwinkelAreaOffset;
        PdufloatArray[4] = soaapUiDaten[chn].pitchwinkelAreaMin;
        PdufloatArray[5] = soaapUiDaten[chn].pitchwinkelAreaMax;

        PdufloatArray[6] = soaapUiDaten[chn].yawwinkelAreaOffset;
        PdufloatArray[7] = soaapUiDaten[chn].yawwinkelAreaMin;
        PdufloatArray[8] = soaapUiDaten[chn].yawwinkelAreaMax;
    }

    public void setPdubyteArray(int chn)
    {
        if(chn < 0) return;
        if(chn > 16) return;

        PdubyteArray[0] = soaapUiDaten[chn].midiAreaNoteTypeHigh;
        PdubyteArray[1] = soaapUiDaten[chn].midiAreaNoteTypeLow;

        PdubyteArray[2] = soaapUiDaten[chn].midiAreaNoteValHigh;
        PdubyteArray[3] = soaapUiDaten[chn].midiAreaNoteValLow;

        PdubyteArray[4] = soaapUiDaten[chn].midiAreaNoteVelHigh;
        PdubyteArray[5] = soaapUiDaten[chn].midiAreaNoteVelLow;
    }


    // -------------------------------------------------------------------------
    // region Telegrammkonstruktion
    // -------------------------------------------------------------------------
    // Das Telegramm wird in einer Zustandsmaschine zusammengesetzt

    enum CrPDUStatus
    {
        Header,
        Time,
        ValueHeader,
        IntValues,
        FloatValues,
        ByteValues,
        NrOfStates
    }

    public String createPDUMsg;

    CrPDUStatus crPDUStatus = CrPDUStatus.Header;
    String HeaderMsg = "SOAAP_V_2";
    boolean testDebug = false;

    //@SuppressLint("SimpleDateFormat");
    public boolean createPDU()
    {
        boolean ready = false;

        if(testDebug)
        {
            createPDUMsg = "Einfach nur ein Telegram zu testen";
            return(true);
        }

        switch(crPDUStatus)
        {
            case Header:
                System.out.println("Header");
                createPDUMsg = HeaderMsg;
                crPDUStatus = CrPDUStatus.ValueHeader;
                break;
            case ValueHeader:
                System.out.println("ValueHeader");
                int intLen, floatLen, byteLen;

                if(PduintArray == null)
                    intLen = 0;
                else
                    intLen = PduintArray.length;

                if(PdufloatArray == null)
                    floatLen = 0;
                else
                    floatLen = PdufloatArray.length;

                if(PdubyteArray == null)
                    byteLen = 0;
                else
                    byteLen = PdubyteArray.length;

                createPDUMsg += ";" + intLen + ";" + floatLen + ";" + byteLen;
                crPDUStatus = CrPDUStatus.IntValues;
                break;

            case IntValues:
                System.out.println("int");
                if(PduintArray != null)
                {
                    for(int i = 0; i < PduintArray.length; i++)
                        createPDUMsg += ";" + PduintArray[i];
                }
                crPDUStatus = CrPDUStatus.FloatValues;
                break;

            case FloatValues:
                System.out.println("float");
                if(PdufloatArray != null)
                {
                    for(int i = 0; i < PdufloatArray.length; i++)
                        createPDUMsg += ";" + PdufloatArray[i];
                }
                crPDUStatus = CrPDUStatus.ByteValues;
                break;

            case ByteValues:
                System.out.println("byte");
                String ByteToString;
                ByteToString = getByteString(PdubyteArray);
                createPDUMsg += ";" + ByteToString;
                createPDUMsg += "::";                      // Beim der Verarbeitung des Telegramms wird das letzte Zeichen
                                                            // mit #00 überschrieben, um einen C-String zu generieren.
                                                            // Hier wird '0' als entsprechender Platzhalter angehängt
                ready = true;
                crPDUStatus = CrPDUStatus.Header;
                break;

            default:
                ready = true;
                crPDUStatus = CrPDUStatus.Header;
                break;
        }

        return ready;
    }

    public String getPDU()
    {
        return(createPDUMsg);
    }


    //--------------------------------------------

    public boolean createPdu_seprate()
    {
        boolean ready = false;
        // <TODO>



        return ready;
    }




    // -------------------------------------------------------------------------
    // Extra Anwedungensfunktionen
    // -------------------------------------------------------------------------
    //

    public boolean Connect()
    {
        try
        {
            socket = new Socket("192.168.4.1", 4210);
            // Ob der Client mit dem Server erfolgreich verbunden ist
            System.out.println(socket.isConnected());
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }

        return socket.isConnected();
    }

    public void Disconnect()
    {
        try {
            outputStream.close();
            socket.close();
            System.out.println(socket.isConnected());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void send()
    {
        // create PDU

        boolean fin = false;
        do
        {
            fin = createPDU();
        }while(fin == false);
        System.out.println(createPDUMsg);

        sendBuffer = createPDUMsg.getBytes();

        try {
            outputStream = socket.getOutputStream();
            outputStream.write(sendBuffer);
            outputStream.flush();
        }
        catch(IOException e)
        {
            e.printStackTrace();
        }
    }

    public void receive()
    {
        try {
            is = socket.getInputStream();

            // Instanz des InputStream erstellen,
            isr = new InputStreamReader(is);
            br = new BufferedReader(isr);
            response = br.readLine();    // emfangsdaten

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    //----------------------------------------------------------


}




