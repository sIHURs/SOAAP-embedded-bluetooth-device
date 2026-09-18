package hsh.mplab.soaapinterface;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Path;
import android.os.Bundle;


// UI Components
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Toast;

import com.google.android.material.button.MaterialButtonToggleGroup;
import com.google.android.material.slider.LabelFormatter;
import com.google.android.material.slider.RangeSlider;                        //Material Components - Sliders
import com.google.android.material.slider.Slider;                             //Material Components - Sliders

import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;


import android.os.Handler;
import android.os.Message;

import java.lang.reflect.Parameter;
import java.text.NumberFormat;
import java.util.Currency;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;




public class MainActivity<Private> extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mThreadPool = Executors.newCachedThreadPool();   // Thread Pool starten

//        // tcp rec test
//
//        String                      response; // data vom Server
//
//        InputStream is; // input Stream
//        InputStreamReader isr ; // input Stream auslesen
//        BufferedReader br ; // read buffer
//
//        // Init des main Threads
//        mMainHandler = new Handler() {
//            @Override
//            public void handleMessage(Message msg) {
//                switch (msg.what) {
//                    case 0:
//                        receive_message.setText(response);
//                        break;
//                }
//            }
//        };
//
//        mThreadPool.execute(new Runnable() {
//            @Override
//            public void run() {
//                try {
//                    is = socket.getInputStream();
//
//                    // Instanz des InputStream erstellen,
//                    isr = new InputStreamReader(is);
//                    br = new BufferedReader(isr);
//                    response = br.readLine();
//
//                    // Darstellung Msg
//                    Message msg = Message.obtain();
//                    msg.what = 0;
//                    mMainHandler.sendMessage(msg);
//
//                } catch (IOException e) {
//                    e.printStackTrace();
//                }
//            }
//        });

        SoaapClientInit();
        graphInit();
        graphBetrieb();

        mThreadPool.execute(new Runnable() {
            @Override
            public void run() {
                timerInit();
            }
        });

        // APPLY Button Callback Funktion
        // Da der Socket in dieser Funktion aufgerufen wird, kann die nicht im UI-Thread geschrieben werden,
        // sondern einen neuen Thread von ThreadPool wird dafuer gestartet
        applyBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mThreadPool.execute(new Runnable() {
                    @Override
                    public void run() {
                        soaapClient.setPduDatensatz(CurrentChannel);
                        soaapClient.send();
                    }
                });
                //System.out.println(sendBuffer);
            }
        });



    }       //onCreate

    // =========================================================================
    // Variablen
    // =========================================================================

    private Handler mMainHandler;  // main Thread - die Funktion, um die Daten vom Server auszudruecken
    private ExecutorService mThreadPool; // Thread

    // UI - Componenten
    private RangeSlider rollAreaRangeSlider, pitchAreaRangeSlider, yawAreaRangeSlider;
    private RangeSlider midiAreaNoteTypeRangeSlider, midiAreaNoteValRangeSlider, midiAreaNoteVelRangeSlider;
    private Slider rollOffsetSilder, pitchOffsetSilder, yawOffsetSilder;

    private MaterialButtonToggleGroup aimRollToggleGroup, aimPitchToggleGroup, aimYawToggleGroup;
    private MaterialButtonToggleGroup mapRollToggleGroup, mapPitchToggleGroup, mapYawToggleGroup;
    private MaterialButtonToggleGroup opModeToggleGroup;

    private AutoCompleteTextView chnSelectAutoCompleteTextView;

    private Button showBottomSheetBtn;
    private Button applyBtn;

    private SOAAPClient soaapClient;

    //Extra Test Buttons -- nicht im Betrieb
    private Button btnConnect, btnDisconnect;
    private Button btnPreSet1;
    private Button btnDebug;
    private TextView Receive,receive_message;

    String[] Subjects = new String[]{"Channel 0", "Channel 1", "Channel 2", "Channel 3",
                                     "Channel 4", "Channel 5", "Channel 6", "Channel 7",
                                     "Channel 8", "Channel 9", "Channel 10", "Channel 11",
                                     "Channel 12", "Channel 13", "Channel 14", "Channel 15"};

    int CurrentChannel = 0;

    // =========================================================================
    // Fukntionen
    // =========================================================================

    public void SoaapClientInit()
    {
        soaapClient = new SOAAPClient(SOAAPClient.SOAAP_UI_VERSION_NR.SOAAP_UI_1_Prof_Patzke_Konfiguration);
    }

    // Timer FUnktion
    Timer     SoaapClientTimer;
    TimerTask SoaapClientTimerTask;
    int       frequency;

    private void timerInit()
    {
        long TimerPeriod = 10;      // repetition time in milliseconds
        long TimerStartDelay = 300; // start delay in milliseconds
        // Expecting some time extra for creating
        // all graphics we simply wait

        frequency = (int) (1000/ TimerPeriod);  // Timer frequency is put to a
        // global variable for using it
        // in any enviroment

        SoaapClientTimerTask = new TimerTask() {
            @Override
            public void run() {
                soaapClient.run(frequency);
            }
        };
        SoaapClientTimer = new Timer();
        SoaapClientTimer.scheduleAtFixedRate(SoaapClientTimerTask, TimerStartDelay, TimerPeriod);
        // Einrichten und Parametrieren eines Timers
    }


    // =========================================================================
    // Graphic (GUI) Initialisierung
    // =========================================================================

    public void graphInit()
    {
        rollAreaRangeSlider = (RangeSlider) findViewById(R.id.RollAreaSlider);
        pitchAreaRangeSlider = (RangeSlider) findViewById(R.id.PitchAreaSlider);
        yawAreaRangeSlider = (RangeSlider) findViewById(R.id.YawAreaSlider);

        rollOffsetSilder = (Slider) findViewById(R.id.RollOffsetSlider);
        pitchOffsetSilder = (Slider) findViewById(R.id.PitchOffsetSlider);
        yawOffsetSilder = (Slider) findViewById(R.id.YawOffsetSlider);

        midiAreaNoteTypeRangeSlider = (RangeSlider) findViewById(R.id.MidiNoteTypeAreaRangeSlider);
        midiAreaNoteValRangeSlider = (RangeSlider) findViewById(R.id.MidiNoteValAreaRangeSlider);
        midiAreaNoteVelRangeSlider = (RangeSlider) findViewById(R.id.MidiNoteVelAreaRangeSlider);

        aimRollToggleGroup = (MaterialButtonToggleGroup) findViewById(R.id.AimRollToggleButton);
        aimPitchToggleGroup = (MaterialButtonToggleGroup) findViewById(R.id.AimPitchToggleButton);
        aimYawToggleGroup = (MaterialButtonToggleGroup) findViewById(R.id.AimYawToggleButton);

        mapRollToggleGroup = (MaterialButtonToggleGroup) findViewById(R.id.MapRollToggleButton);
        mapPitchToggleGroup = (MaterialButtonToggleGroup) findViewById(R.id.MapPitchToggleButton);
        mapYawToggleGroup = (MaterialButtonToggleGroup) findViewById(R.id.MapYawToggleButton);

        opModeToggleGroup = (MaterialButtonToggleGroup) findViewById(R.id.OpModeToggleButton);

        showBottomSheetBtn =  (Button) findViewById(R.id.ChannelSelectBtn);
        applyBtn = (Button) findViewById(R.id.ApplyBtn);
        chnSelectAutoCompleteTextView = findViewById(R.id.Chn1Spinner);
    }

    // =========================================================================
    // UI - Componenten Callback
    // =========================================================================

    public void graphBetrieb()
    {
        // =========================================================================
        // Dropdown Menu - Channel auswaehlen

        // create an array adapter and pass the required parameter
        // in our case pass the context, drop down layout , and array.
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.dropdown_item, Subjects);
        chnSelectAutoCompleteTextView.setAdapter(adapter);

        //to get selected value add item click listener
        chnSelectAutoCompleteTextView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String s = chnSelectAutoCompleteTextView.getText().toString();
                Toast.makeText(getApplicationContext(), "select" + s, Toast.LENGTH_SHORT).show();
                switch (s)
                {
                    case "Channel 0":
                        CurrentChannel = 0;
                        break;

                    case "Channel 1":
                        CurrentChannel = 1;
                        break;

                    case "Channel 2":
                        CurrentChannel = 2;
                        break;

                    case "Channel 3":
                        CurrentChannel = 3;
                        break;

                    case "Channel 4":
                        CurrentChannel = 4;
                        break;

                    case "Channel 5":
                        CurrentChannel = 5;
                        break;

                    case "Channel 6":
                        CurrentChannel = 6;
                        break;

                    case "Channel 7":
                        CurrentChannel = 7;
                        break;

                    case "Channel 8":
                        CurrentChannel = 8;
                        break;

                    case "Channel 9":
                        CurrentChannel = 9;
                        break;

                    case "Channel 10":
                        CurrentChannel = 10;
                        break;

                    case "Channel 11":
                        CurrentChannel = 11;
                        break;

                    case "Channel 12":
                        CurrentChannel = 12;
                        break;

                    case "Channel 13":
                        CurrentChannel = 13;
                        break;

                    case "Channel 14":
                        CurrentChannel = 14;
                        break;

                    case "Channel 15":
                        CurrentChannel = 15;
                        break;

                    default:
                        break;
                }
                soaapClient.setChannel(CurrentChannel);
                setCurrentGraph(CurrentChannel);
                System.out.println(CurrentChannel);
            }
        });

        // =========================================================================
        // Sliders

        rollAreaRangeSlider.addOnChangeListener(new RangeSlider.OnChangeListener() {
            float new_value;
            @Override
            public void onValueChange(@NonNull RangeSlider slider, float value, boolean fromUser) {
                new_value = value;
                if(new_value< 50)
                {
                    soaapClient.setRollAreaMin(CurrentChannel, new_value);
                }
                else
                {
                    soaapClient.setRollAreaMax(CurrentChannel, new_value);
                }
            }
        });

        rollOffsetSilder.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                soaapClient.setRollAreaOffset(CurrentChannel, value);
            }
        });

        pitchAreaRangeSlider.addOnChangeListener(new RangeSlider.OnChangeListener() {
            float new_value;
            @Override
            public void onValueChange(@NonNull RangeSlider slider, float value, boolean fromUser) {
                new_value = value;
                if(new_value< 50)
                {
                    soaapClient.setPitchAreaMin(CurrentChannel, new_value);
                }
                else
                {
                    soaapClient.setPitchAreaMax(CurrentChannel, new_value);
                }
            }
        });

        pitchOffsetSilder.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                soaapClient.setPitchAreaOffset(CurrentChannel, value);
            }
        });

        yawAreaRangeSlider.addOnChangeListener(new RangeSlider.OnChangeListener() {
            float new_value;
            @Override
            public void onValueChange(@NonNull RangeSlider slider, float value, boolean fromUser) {
                new_value = value;
                if(new_value< 50)
                {
                    soaapClient.setYawAreaMin(CurrentChannel, new_value);
                }
                else
                {
                    soaapClient.setYawAreaMax(CurrentChannel, new_value);
                }
            }
        });

        yawOffsetSilder.addOnChangeListener(new Slider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull Slider slider, float value, boolean fromUser) {
                soaapClient.setYawAreaOffset(CurrentChannel, value);
            }
        });

        midiAreaNoteTypeRangeSlider.addOnChangeListener(new RangeSlider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull RangeSlider slider, float value, boolean fromUser) {
                if(value < 50)
                {
                    soaapClient.setMidiNoteTypeAreaHigh(CurrentChannel, (byte) value);
                }
                else
                {
                    soaapClient.setMidiNoteTypeAreaLow(CurrentChannel, (byte) value);
                }
            }
        });

        midiAreaNoteValRangeSlider.addOnChangeListener(new RangeSlider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull RangeSlider slider, float value, boolean fromUser) {
                if(value < 50)
                {
                    soaapClient.setMidiNoteValAreaHigh(CurrentChannel, (byte) value);
                }
                else
                {
                    soaapClient.setMidiNoteValAreaLow(CurrentChannel, (byte) value);
                }
            }
        });

        midiAreaNoteVelRangeSlider.addOnChangeListener(new RangeSlider.OnChangeListener() {
            @Override
            public void onValueChange(@NonNull RangeSlider slider, float value, boolean fromUser) {
                if(value < 50)
                {
                    soaapClient.setMidiNoteVelAreaHigh(CurrentChannel, (byte) value);
                }
                else
                {
                    soaapClient.setMidiNoteVelAreaLow(CurrentChannel, (byte) value);
                }
            }
        });

        // =========================================================================
        // ToggleButtonGroup

        aimRollToggleGroup.addOnButtonCheckedListener(new MaterialButtonToggleGroup.OnButtonCheckedListener() {
            @Override
            public void onButtonChecked(MaterialButtonToggleGroup group, int checkedId, boolean isChecked) {
                if(isChecked)
                {
                    switch (checkedId)
                    {
                        case R.id.AimRollNothingBtn:
                            soaapClient.setAimRoll(CurrentChannel, SOAAPClient.Meas2Midi.Nothing);
                            break;

                        case R.id.AimRollNoteTypeBtn:
                            soaapClient.setAimRoll(CurrentChannel, SOAAPClient.Meas2Midi.NoteType);
                            break;

                        case R.id.AimRollNoteValBtn:
                            soaapClient.setAimRoll(CurrentChannel, SOAAPClient.Meas2Midi.NoteVal);
                            break;

                        case R.id.AimRollNoteVelBtn:
                            soaapClient.setAimRoll(CurrentChannel, SOAAPClient.Meas2Midi.NoteVel);
                            break;

                        default:
                            break;
                    }
                }
            }
        });

        aimPitchToggleGroup.addOnButtonCheckedListener(new MaterialButtonToggleGroup.OnButtonCheckedListener() {
            @Override
            public void onButtonChecked(MaterialButtonToggleGroup group, int checkedId, boolean isChecked) {
                if(isChecked)
                {
                    switch (checkedId)
                    {
                        case R.id.AimPitchNothingBtn:
                            soaapClient.setAimPitch(CurrentChannel, SOAAPClient.Meas2Midi.Nothing);
                            break;

                        case R.id.AimPitchNoteTypeBtn:
                            soaapClient.setAimPitch(CurrentChannel, SOAAPClient.Meas2Midi.NoteType);
                            break;

                        case R.id.AimPitchNoteValBtn:
                            soaapClient.setAimPitch(CurrentChannel, SOAAPClient.Meas2Midi.NoteVal);
                            break;

                        case R.id.AimPitchNoteVelBtn:
                            soaapClient.setAimPitch(CurrentChannel, SOAAPClient.Meas2Midi.NoteVel);
                            break;

                        default:
                            break;
                    }
                }
            }
        });

        aimYawToggleGroup.addOnButtonCheckedListener(new MaterialButtonToggleGroup.OnButtonCheckedListener() {
            @Override
            public void onButtonChecked(MaterialButtonToggleGroup group, int checkedId, boolean isChecked) {
                if(isChecked)
                {
                    switch (checkedId)
                    {
                        case R.id.AimYawNothingBtn:
                            soaapClient.setAimYaw(CurrentChannel, SOAAPClient.Meas2Midi.Nothing);
                            break;

                        case R.id.AimYawNoteTypeBtn:
                            soaapClient.setAimYaw(CurrentChannel, SOAAPClient.Meas2Midi.NoteType);
                            break;

                        case R.id.AimYawNoteValBtn:
                            soaapClient.setAimYaw(CurrentChannel, SOAAPClient.Meas2Midi.NoteVal);
                            break;

                        case R.id.AimYawNoteVelBtn:
                            soaapClient.setAimYaw(CurrentChannel, SOAAPClient.Meas2Midi.NoteVel);
                            break;

                        default:
                            break;
                    }
                }
            }
        });

        mapRollToggleGroup.addOnButtonCheckedListener(new MaterialButtonToggleGroup.OnButtonCheckedListener() {
            @Override
            public void onButtonChecked(MaterialButtonToggleGroup group, int checkedId, boolean isChecked) {
                if(isChecked)
                {
                    switch (checkedId)
                    {
                        case R.id.MapRollOtoOgBtn:
                            soaapClient.setMappingRoll(CurrentChannel, SOAAPClient.MeasMap.OneToOne);
                            break;

                        case R.id.MapRollBiBtn:
                            soaapClient.setMappingRoll(CurrentChannel, SOAAPClient.MeasMap.BiLinear);
                            break;

                        default:
                            break;
                    }
                }
            }
        });

        mapPitchToggleGroup.addOnButtonCheckedListener(new MaterialButtonToggleGroup.OnButtonCheckedListener() {
            @Override
            public void onButtonChecked(MaterialButtonToggleGroup group, int checkedId, boolean isChecked) {
                if(isChecked)
                {
                    switch (checkedId)
                    {
                        case R.id.MapPitchOtoOgBtn:
                            soaapClient.setMappingPitch(CurrentChannel, SOAAPClient.MeasMap.OneToOne);
                            break;

                        case R.id.MapPitchBiBtn:
                            soaapClient.setMappingPitch(CurrentChannel, SOAAPClient.MeasMap.BiLinear);
                            break;

                        default:
                            break;
                    }
                }
            }
        });

        mapYawToggleGroup.addOnButtonCheckedListener(new MaterialButtonToggleGroup.OnButtonCheckedListener() {
            @Override
            public void onButtonChecked(MaterialButtonToggleGroup group, int checkedId, boolean isChecked) {
                if(isChecked)
                {
                    switch (checkedId)
                    {
                        case R.id.MapYawOtoOgBtn:
                            soaapClient.setMappingYaw(CurrentChannel, SOAAPClient.MeasMap.OneToOne);
                            break;

                        case R.id.MapYawBiBtn:
                            soaapClient.setMappingYaw(CurrentChannel, SOAAPClient.MeasMap.BiLinear);
                            break;

                        default:
                            break;
                    }
                }
            }
        });

        opModeToggleGroup.addOnButtonCheckedListener(new MaterialButtonToggleGroup.OnButtonCheckedListener() {
            @Override
            public void onButtonChecked(MaterialButtonToggleGroup group, int checkedId, boolean isChecked) {
                if(isChecked)
                {
                    switch (checkedId)
                    {
                        case R.id.OpModeIdleBtn:
                            soaapClient.setOpMode(CurrentChannel, SOAAPClient.MidiOpMode.momIdel);
                            break;

                        case R.id.OpModeSeqBtn:
                            soaapClient.setOpMode(CurrentChannel, SOAAPClient.MidiOpMode.momSequence);
                            break;

                        case R.id.OpModeRunBtn:
                            soaapClient.setOpMode(CurrentChannel, SOAAPClient.MidiOpMode.momRunDealt);
                            break;

                        default:
                            break;
                    }
                }
            }
        });


        // =========================================================================
        // open Bottom Sheet
        showBottomSheetBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v)
            {
                BottomSheetDialog bottomSheet = new BottomSheetDialog();
                bottomSheet.show(getSupportFragmentManager(), "ModalBottomSheet");
            }
        });


    } // End - grphBetrieb()

    public void setCurrentGraph(int chn)
    {
        rollAreaRangeSlider.setValues(soaapClient.getRollAreaMin(chn), soaapClient.getRollAreaMax(chn));
        rollOffsetSilder.setValue(soaapClient.getRollAreaOffset(chn));
        pitchAreaRangeSlider.setValues(soaapClient.getPitchAreaMin(chn), soaapClient.getPitchAreaMax(chn));
        pitchOffsetSilder.setValue(soaapClient.getPitchAreaOffset(chn));
        yawAreaRangeSlider.setValues(soaapClient.getYawAreaMin(chn), soaapClient.getYawAreaMax(chn));
        yawOffsetSilder.setValue(soaapClient.getYawAreaOffset(chn));


        midiAreaNoteTypeRangeSlider.setValues(oneByteToFloat(soaapClient.getMidiNoteTypeAreaLow(chn)),
                                              oneByteToFloat(soaapClient.getMidiNoteTypeAreaHigh(chn)));
        midiAreaNoteValRangeSlider.setValues(oneByteToFloat(soaapClient.getMidiNoteValAreaLow(chn)),
                                             oneByteToFloat(soaapClient.getMidiNoteValAreaHigh(chn)));
        midiAreaNoteVelRangeSlider.setValues(oneByteToFloat(soaapClient.getMidiNoteVelAreaLow(chn)),
                                             oneByteToFloat(soaapClient.getMidiNoteVelAreaHigh(chn)));


        switch (soaapClient.getAimRoll(chn))
        {
            case Nothing:
                aimRollToggleGroup.check(R.id.AimRollNothingBtn);
                break;

            case NoteType:
                aimRollToggleGroup.check(R.id.AimRollNoteTypeBtn);
                break;

            case NoteVal:
                aimRollToggleGroup.check(R.id.AimRollNoteValBtn);
                break;

            case NoteVel:
                aimRollToggleGroup.check(R.id.AimRollNoteVelBtn);
                break;

            default:
                break;
        }

        switch (soaapClient.getAimPitch(chn))
        {
            case Nothing:
                aimPitchToggleGroup.check(R.id.AimPitchNothingBtn);
                break;

            case NoteType:
                aimPitchToggleGroup.check(R.id.AimPitchNoteTypeBtn);
                break;

            case NoteVal:
                aimPitchToggleGroup.check(R.id.AimPitchNoteValBtn);
                break;

            case NoteVel:
                aimPitchToggleGroup.check(R.id.AimPitchNoteVelBtn);
                break;

            default:
                break;
        }

        switch (soaapClient.getAimYaw(chn))
        {
            case Nothing:
                aimYawToggleGroup.check(R.id.AimYawNothingBtn);
                break;

            case NoteType:
                aimYawToggleGroup.check(R.id.AimYawNoteTypeBtn);
                break;

            case NoteVal:
                aimYawToggleGroup.check(R.id.AimYawNoteValBtn);
                break;

            case NoteVel:
                aimYawToggleGroup.check(R.id.AimYawNoteVelBtn);
                break;

            default:
                break;
        }

        switch (soaapClient.getMappingRoll(chn))
        {
            case OneToOne:
                mapRollToggleGroup.check(R.id.MapRollOtoOgBtn);
                break;

            case BiLinear:
                mapRollToggleGroup.check(R.id.MapRollBiBtn);

            default:
                break;
        }

        switch (soaapClient.getMappingPitch(chn))
        {
            case OneToOne:
                mapPitchToggleGroup.check(R.id.MapPitchOtoOgBtn);
                break;

            case BiLinear:
                mapPitchToggleGroup.check(R.id.MapPitchBiBtn);

            default:
                break;
        }

        switch (soaapClient.getMappingYaw(chn))
        {
            case OneToOne:
                mapYawToggleGroup.check(R.id.MapYawOtoOgBtn);
                break;

            case BiLinear:
                mapYawToggleGroup.check(R.id.MapYawBiBtn);

            default:
                break;
        }

        switch (soaapClient.getOpMode(chn))
        {
            case momIdel:
                opModeToggleGroup.check(R.id.OpModeIdleBtn);
                break;

            case momSequence:
                opModeToggleGroup.check(R.id.OpModeSeqBtn);
                break;

            case momRunDealt:
                opModeToggleGroup.check(R.id.OpModeRunBtn);
                break;

            default:
                break;
        }
    }


    // =========================================================================
    // print - Debug
    // =========================================================================

    private void datensatzPrintDebug(int chn)
    {
        System.out.println("channel: " + soaapClient.soaapUiDaten[chn].channel);
        System.out.println("rollwinkelAreaOffset: " + soaapClient.soaapUiDaten[chn].rollwinkelAreaOffset);
        System.out.println("rollwinkelAreaMin: " + soaapClient.soaapUiDaten[chn].rollwinkelAreaMin);
        System.out.println("rollwinkelAreaMax: " + soaapClient.soaapUiDaten[chn].rollwinkelAreaMax);
        System.out.println("pitchwinkelAreaOffset: " + soaapClient.soaapUiDaten[chn].pitchwinkelAreaOffset);
        System.out.println("pitchwinkelAreaMin: " + soaapClient.soaapUiDaten[chn].pitchwinkelAreaMin);
        System.out.println("pitchwinkelAreaMax: " + soaapClient.soaapUiDaten[chn].pitchwinkelAreaMax);
        System.out.println("yawwinkelAreaOffset: " + soaapClient.soaapUiDaten[chn].yawwinkelAreaOffset);
        System.out.println("yawwinkelAreaMin: " + soaapClient.soaapUiDaten[chn].yawwinkelAreaMin);
        System.out.println("yawwinkelAreaMax: " + soaapClient.soaapUiDaten[chn].yawwinkelAreaMax);

        System.out.println("aimRoll: " + soaapClient.soaapUiDaten[chn].aimRoll);
        System.out.println("aimPitch: " + soaapClient.soaapUiDaten[chn].aimPitch);
        System.out.println("aimYaw: " + soaapClient.soaapUiDaten[chn].aimYaw);

        System.out.println("mapRoll: " + soaapClient.soaapUiDaten[chn].mapRoll);
        System.out.println("mapPitch: " + soaapClient.soaapUiDaten[chn].mapPitch);
        System.out.println("mapYaw: " + soaapClient.soaapUiDaten[chn].mapYaw);

        System.out.println("midiOpMode: " + soaapClient.soaapUiDaten[chn].midiOpMode);

        System.out.println("midiAreaNoteTypeHigh: " + soaapClient.soaapUiDaten[chn].midiAreaNoteTypeHigh);
        System.out.println("midiAreaNoteTypeLow: " + soaapClient.soaapUiDaten[chn].midiAreaNoteTypeLow);
        System.out.println("midiAreaNoteValHigh: " + soaapClient.soaapUiDaten[chn].midiAreaNoteValHigh);
        System.out.println("midiAreaNoteValLow: " + soaapClient.soaapUiDaten[chn].midiAreaNoteValLow);
        System.out.println("midiAreaNoteVelHigh: " + soaapClient.soaapUiDaten[chn].midiAreaNoteVelHigh);
        System.out.println("midiAreaNoteVelLow: " + soaapClient.soaapUiDaten[chn].midiAreaNoteVelLow);
    }


    // =========================================================================
    // Hilfsfunktion
    // =========================================================================

    public float oneByteToFloat(byte input)
    {
        return Float.intBitsToFloat(input & 0xFF);
    }

}       //MainActivity