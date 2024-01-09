package com.eliedersousa.hud

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.felhr.usbserial.UsbSerialDevice
import com.felhr.usbserial.UsbSerialInterface
import com.felhr.usbserial.UsbSerialInterface.UsbReadCallback
import java.io.UnsupportedEncodingException


class MainActivity : AppCompatActivity() {
    private fun showMessage( msg: String ) {
        //Toast.makeText(this, msg, Toast.LENGTH_LONG).show()
        val text_debug = findViewById<TextView>(R.id.text_debug)
        text_debug.text = text_debug.text.toString() + msg + "\n"
    }

    private val mCallback = UsbReadCallback {arg0 ->
        try {
            runOnUiThread {
                val s = String(arg0)
                showMessage( "Mensagem: " + s)
            }
        } catch(e: UnsupportedEncodingException) {
            //e.printStackTrace()
            runOnUiThread {
                val s = String(arg0)
                showMessage( "Mensagem: " + s)
            }
        }
    }

    lateinit var m_usbManager: UsbManager
    var m_device: UsbDevice? = null
    var m_serial: UsbSerialDevice? = null
    var m_connection: UsbDeviceConnection? = null
    var ACTION_USB_PERMISSION = "com.eliedersousa.hud.USB_PERMISSION"

    private val broadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action!! == ACTION_USB_PERMISSION) {

                val granted: Boolean = intent.extras!!.getBoolean(UsbManager.EXTRA_PERMISSION_GRANTED)
                if (granted) {
                    m_connection = m_usbManager.openDevice(m_device)
                    m_serial = UsbSerialDevice.createUsbSerialDevice(m_device, m_connection)
                    showMessage("Broadcast receiver " + m_connection.toString())
                    if (m_serial != null) {
                        if (m_serial!!.open()) {
                            m_serial!!.setBaudRate(115200)
                            m_serial!!.setDataBits(UsbSerialInterface.DATA_BITS_8)
                            m_serial!!.setStopBits(UsbSerialInterface.STOP_BITS_1)
                            m_serial!!.setParity(UsbSerialInterface.PARITY_NONE)
                            m_serial!!.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF)
                            m_serial!!.read(mCallback)
                            showMessage("Conexão iniciada!")
                        } else {
                            showMessage("Porta não está aberta")
                        }
                    } else {
                        showMessage( "Porta é null")
                    }
                } else {
                    showMessage("Permissão não foi concedida")
                }
            } else if (intent.action == UsbManager.ACTION_USB_ACCESSORY_ATTACHED) {
                startUsbConnecting()
                showMessage("USB conectada")
            } else if (intent.action == UsbManager.ACTION_USB_ACCESSORY_DETACHED) {
                showMessage("USB desconectada")
                m_serial?.close()
            }
        }
    }

    private fun testData() {
        sendData("k")
    }

    private fun sendData( input: String) {
        m_serial?.write(input.toByteArray())
        showMessage("Sended Msg: " + input.toByteArray())
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        // View Instructions
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        m_usbManager = getSystemService(USB_SERVICE) as UsbManager
        //val btn_teste = findViewById<Button>(R.id.btn_teste)
        //btn_teste.setOnClickListener( {testData()} )

        val filter = IntentFilter()
        filter.addAction(ACTION_USB_PERMISSION)
        filter.addAction(UsbManager.ACTION_USB_ACCESSORY_ATTACHED)
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
        startUsbConnecting()
        registerReceiver(broadcastReceiver, filter)
    }

    private fun startUsbConnecting() {
        val usbDevices = m_usbManager.deviceList ?: return // Return if no devices found
        usbDevices.forEach { entry ->
            val device = entry.value
            val deviceVendorId = device.vendorId ?: return@forEach // Skip if vendor ID is null

            if (deviceVendorId == 6790) { // 6790 é o Arduino UNO
                val intent = PendingIntent.getBroadcast(this, 0, Intent(ACTION_USB_PERMISSION), 0)
                m_usbManager.requestPermission(device, intent)
                m_device = device
                showMessage("Connection successful with vendorID: $deviceVendorId")
                return
            }
        }
        showMessage( "No matching USB device found!")
    }
}