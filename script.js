 // DOM Elements
   
	  const connectButton = document.getElementById('connectBleButton');
    const disconnectButton = document.getElementById('disconnectBleButton');
    
	  const prideButton = document.getElementById('prideButton');
	  const wavesButton = document.getElementById('wavesButton');
	  const rainbowButton = document.getElementById('rainbowButton');
    const bubbleButton = document.getElementById('bubbleButton');
    const dotsButton = document.getElementById('dotsButton');
    const fxWave2dButton = document.getElementById('fxWave2dButton');
    const radiiButton = document.getElementById('radiiButton');
    const cellsButton = document.getElementById('cellsButton');
    const offButton = document.getElementById('offButton');

    const octopusButton = document.getElementById('octopusButton');
    const flowerButton = document.getElementById('flowerButton');
    const lotusButton = document.getElementById('lotusButton');
    const radialButton = document.getElementById('radialButton');
	
    //const fixedPaletteButton = document.getElementById('fixedPaletteButton');
    //const rotatePaletteButton = document.getElementById('rotatePaletteButton');
    const rotatePaletteCheckbox = document.getElementById('rotatePaletteCheckbox');
   
    const setPalNumForm = document.getElementById('PalNumForm');
    const PalNumInput = document.getElementById('PalNumInput');
      
    const brighterButton = document.getElementById('brighterButton');
  	const dimmerButton = document.getElementById('dimmerButton');
    const fasterButton = document.getElementById('fasterButton');
	  const slowerButton = document.getElementById('slowerButton');
	
    const fancyButton = document.getElementById('fancyButton');

	  const latestValueSent = document.getElementById('valueSent');
    const bleStateContainer = document.getElementById('bleState');

    //Define BLE Device Specs
    var deviceName ='Aurora Portal';
    var bleService =                '19b10000-e8f2-537e-4f6c-d104768a1214';
    var ProgramCharacteristic =     '19b10001-e8f2-537e-4f6c-d104768a1214';
    var ModeCharacteristic =        '19b10002-e8f2-537e-4f6c-d104768a1214';
    var BrightnessCharacteristic =  '19b10003-e8f2-537e-4f6c-d104768a1214';
	  var SpeedCharacteristic =       '19b10004-e8f2-537e-4f6c-d104768a1214';
    var PaletteCharacteristic =     '19b10005-e8f2-537e-4f6c-d104768a1214';
    var ControlCharacteristic =     '19b10006-e8f2-537e-4f6c-d104768a1214';
 
    //Global Variables to Handle Bluetooth
    var bleServer;
    var bleServiceFound;
  	var programCharacteristicFound;
	  var modeCharacteristicFound;
	  var brightnessCharacteristicFound;
    var speedCharacteristicFound;
    var paletteCharacteristicFound;
	  var controlCharacteristicFound;

    // Connect Button (search for BLE Devices only if BLE is available)
    connectButton.addEventListener('click', (event) => {
        if (isWebBluetoothEnabled()){
            connectToDevice();
        }
    });

    // Disconnect Button
    disconnectButton.addEventListener('click', disconnectDevice);

    // Write to the Program Characteristic
    rainbowButton.addEventListener('click', () => writeProgramCharacteristic(1));
    prideButton.addEventListener('click', () => writeProgramCharacteristic(2));
	  wavesButton.addEventListener('click', () => writeProgramCharacteristic(3));
	  bubbleButton.addEventListener('click', () => writeProgramCharacteristic(4));
    dotsButton.addEventListener('click', () => writeProgramCharacteristic(5));
    fxWave2dButton.addEventListener('click', () => writeProgramCharacteristic(6));
    radiiButton.addEventListener('click', () => writeProgramCharacteristic(7));
    cellsButton.addEventListener('click', () => writeProgramCharacteristic(8));
    offButton.addEventListener('click', () => writeProgramCharacteristic(99));
	
	// Write to the Mode Characteristic
    octopusButton.addEventListener('click', () => writeModeCharacteristic(1));
    flowerButton.addEventListener('click', () => writeModeCharacteristic(2));
	  lotusButton.addEventListener('click', () => writeModeCharacteristic(3));
	  radialButton.addEventListener('click', () => writeModeCharacteristic(4));
    
	// Write to the Brightness Characteristic	
	  brighterButton.addEventListener('click', () => writeBrightnessCharacteristic(1));
    dimmerButton.addEventListener('click', () => writeBrightnessCharacteristic(2));
    
    // Write to the Speed Characteristic
    fasterButton.addEventListener('click', () => writeSpeedCharacteristic(1));
    slowerButton.addEventListener('click', () => writeSpeedCharacteristic(2));
    	 
	// Write to the Palette Characteristic
	  setPalNumForm.addEventListener('submit', function(event) {
		event.preventDefault();
		const newPalNum = PalNumInput.value;
		console.log('New palette number:', newPalNum);
		writePaletteCharacteristic(newPalNum);
		setPalNumForm.reset();
	});

    // Write to the Control Characteristic	
	  fancyButton.addEventListener('click', () => writeControlCharacteristic(1));
    //fixedPaletteButton.addEventListener('click', () => writeControlCharacteristic(101));
    //rotatePaletteButton.addEventListener('click', () => writeControlCharacteristic(100));

    rotatePaletteCheckbox.addEventListener('change', () => {
      if (rotatePaletteCheckbox.checked) {
        writeControlCharacteristic(100);
        //outputDiv.textContent = 'Checkbox is checked!';
      } else {
        writeControlCharacteristic(101);
        //outputDiv.textContent = 'Checkbox is unchecked!';
      }
    });


	// Check if BLE is available in your Browser
    function isWebBluetoothEnabled() {
        if (!navigator.bluetooth) {
            console.log("Web Bluetooth API is not available in this browser!");
            bleStateContainer.innerHTML = "Web Bluetooth API is not available in this browser!";
            return false
        }
        console.log('Web Bluetooth API supported in this browser.');
        return true
    }

    // Connect to BLE Device and Enable Notifications
    function connectToDevice(){
        console.log('Initializing Bluetooth...');
        navigator.bluetooth.requestDevice({
            filters: [{name: deviceName}],
            optionalServices: [bleService]
        })
        .then(device => {
            console.log('Device Selected:', device.name);
            bleStateContainer.innerHTML = 'Connected to ' + device.name;
            bleStateContainer.style.color = "#24af37";
            device.addEventListener('gattservicedisconnected', onDisconnected);
            return device.gatt.connect();
        })
        .then(gattServer =>{
            bleServer = gattServer;
            console.log("Connected to GATT Server");
            return bleServer.getPrimaryService(bleService);
        })
        .then(service => {
             bleServiceFound = service;
             console.log("Service discovered:", service.uuid);
             service.getCharacteristics().then(characteristics => {
                 characteristics.forEach(characteristic => {
                 console.log('Characteristic UUID:', characteristic.uuid); 
			     });
			 });

             service.getCharacteristic(ProgramCharacteristic)
				.then(characteristic => {
					programCharacteristicFound = characteristic;
					characteristic.addEventListener('characteristicvaluechanged', handleProgramCharacteristicChange);
					characteristic.startNotifications();				
					return characteristic.readValue();
				 })
				.then(value => {
					//console.log("Read value: ", value);
					const decodedValue = new TextDecoder().decode(value);
					console.log("Program: ", decodedValue);
					programValue.innerHTML = decodedValue;
			     })

             service.getCharacteristic(ModeCharacteristic)
				.then(characteristic => {
					modeCharacteristicFound = characteristic;
					characteristic.addEventListener('characteristicvaluechanged', handleModeCharacteristicChange);
					characteristic.startNotifications();				
					return characteristic.readValue();
				 })
				.then(value => {
					//console.log("Read value: ", value);
					const decodedValue = new TextDecoder().decode(value);
					console.log("Mode: ", decodedValue);
					modeValue.innerHTML = decodedValue;
			     })

			 service.getCharacteristic(BrightnessCharacteristic)
				.then(characteristic => {
					brightnessCharacteristicFound = characteristic;
					characteristic.addEventListener('characteristicvaluechanged', handleBrightnessCharacteristicChange);
					characteristic.startNotifications();				
					return characteristic.readValue();
				 })
				.then(value => {
					const decodedValue = new TextDecoder().decode(value);
					console.log("Brightness: ", decodedValue);
					brightnessValue.innerHTML = decodedValue;
			     })	 
		
		     service.getCharacteristic(SpeedCharacteristic)
				 .then(characteristic => {
					speedCharacteristicFound = characteristic;
					characteristic.addEventListener('characteristicvaluechanged', handleSpeedCharacteristicChange);
					characteristic.startNotifications();	
					return characteristic.readValue();
				 })
				 .then(value => {
					//console.log("Read value: ", value);
					const decodedValue = new TextDecoder().decode(value);
					console.log("Speed: ", decodedValue);
					speedValue.innerHTML = decodedValue;
				 })
	
		     service.getCharacteristic(PaletteCharacteristic)
				 .then(characteristic => {
					paletteCharacteristicFound = characteristic;
					characteristic.addEventListener('characteristicvaluechanged', handlePaletteCharacteristicChange);
					characteristic.startNotifications();	
					return characteristic.readValue();
				 })
				 .then(value => {
					//console.log("Read value: ", value);
					const decodedValue = new TextDecoder().decode(value);
					console.log("Palette: ", decodedValue);
					paletteValue.innerHTML = decodedValue;
				 })
	
             service.getCharacteristic(ControlCharacteristic)
				 .then(characteristic => {
					controlCharacteristicFound = characteristic;
					characteristic.addEventListener('characteristicvaluechanged', handleControlCharacteristicChange);
					characteristic.startNotifications();	
					return characteristic.readValue();
				 })
				 .then(value => {
					//console.log("Read value: ", value);
					const decodedValue = new TextDecoder().decode(value);
					console.log("Control: ", decodedValue);
					controlValue.innerHTML = decodedValue;
				 })


	    })
    }
	
//******************************************************************************************************************

    function onDisconnected(event){
        console.log('Device Disconnected:', event.target.device.name);
        bleStateContainer.innerHTML = 'Device disconnected';
        bleStateContainer.style.color = "#d13a30";
        connectToDevice();
    }

    function handleProgramCharacteristicChange(event){
        const newValueReceived = new TextDecoder().decode(event.target.value);
        console.log("Program changed: ", newValueReceived);
        programValue.innerHTML = newValueReceived;
    }

    function handleModeCharacteristicChange(event){
        const newValueReceived = new TextDecoder().decode(event.target.value);
        console.log("Mode changed: ", newValueReceived);
        modeValue.innerHTML = newValueReceived;
    }

    function handleBrightnessCharacteristicChange(event){
        const newValueReceived = new TextDecoder().decode(event.target.value);
        console.log("Brightness changed: ", newValueReceived);
        brightnessValue.innerHTML = newValueReceived;
    }

    function handleSpeedCharacteristicChange(event){
        const newValueReceived = new TextDecoder().decode(event.target.value);
        console.log("Speed changed: ", newValueReceived);
        speedValue.innerHTML = newValueReceived;
    }

    function handlePaletteCharacteristicChange(event){
        const newValueReceived = new TextDecoder().decode(event.target.value);
        console.log("Palette changed: ", newValueReceived);
        paletteValue.innerHTML = newValueReceived;
    }

    function handleControlCharacteristicChange(event){
        const newValueReceived = new TextDecoder().decode(event.target.value);
        console.log("Control changed: ", newValueReceived);
        controlValue.innerHTML = newValueReceived;
    }

    function writeProgramCharacteristic(value){
        if (bleServer && bleServer.connected) {
            bleServiceFound.getCharacteristic(ProgramCharacteristic)
            .then(characteristic => {
                const data = new Uint8Array([value]);
                return characteristic.writeValue(data);
            })
            .then(() => {
                programValue.innerHTML = value;
                latestValueSent.innerHTML = value;
                console.log("Value written to Program characteristic:", value);
            })
            .catch(error => {
                console.error("Error writing to Program characteristic: ", error);
            });
        } else {
            console.error ("Bluetooth is not connected. Cannot write to characteristic.")
            window.alert("Bluetooth is not connected. Cannot write to characteristic. \n Connect to BLE first!")
        }
    }

    function writeModeCharacteristic(value){
        if (bleServer && bleServer.connected) {
            bleServiceFound.getCharacteristic(ModeCharacteristic)
            .then(characteristic => {
//                console.log("Found Mode characteristic: ", characteristic.uuid);
                const data = new Uint8Array([value]);
                return characteristic.writeValue(data);
            })
            .then(() => {
                modeValue.innerHTML = value;
                latestValueSent.innerHTML = value;
                console.log("Value written to Mode characteristic:", value);
            })
            .catch(error => {
                console.error("Error writing to Mode characteristic: ", error);
            });
        } else {
            console.error ("Bluetooth is not connected. Cannot write to characteristic.")
            window.alert("Bluetooth is not connected. Cannot write to characteristic. \n Connect to BLE first!")
        }
    }

	function writeBrightnessCharacteristic(value){
        if (bleServer && bleServer.connected) {
            bleServiceFound.getCharacteristic(BrightnessCharacteristic)
            .then(characteristic => {
                const data = new Uint8Array([value]);
                return characteristic.writeValue(data);
            })
            .then(() => {
                brightnessValue.innerHTML = value;
                latestValueSent.innerHTML = value;
                console.log("Value written to Brightness characteristic:", value);
            })
            .catch(error => {
                console.error("Error writing to Brightness characteristic: ", error);
            });
        } else {
            console.error ("Bluetooth is not connected. Cannot write to characteristic.")
            window.alert("Bluetooth is not connected. Cannot write to characteristic. \n Connect to BLE first!")
        }
    }

    function writeSpeedCharacteristic(value){
        if (bleServer && bleServer.connected) {
            bleServiceFound.getCharacteristic(SpeedCharacteristic)
            .then(characteristic => {
                const data = new Uint8Array([value]);
                return characteristic.writeValue(data);
            })
            .then(() => {
                speedValue.innerHTML = value;
                latestValueSent.innerHTML = value;
                console.log("Value written to Speed characteristic:", value);
            })
            .catch(error => {
                console.error("Error writing to Speed characteristic: ", error);
            });
        } else {
            console.error ("Bluetooth is not connected. Cannot write to characteristic.")
            window.alert("Bluetooth is not connected. Cannot write to characteristic. \n Connect to BLE first!")
        }
    }

	function writePaletteCharacteristic(value){
        if (bleServer && bleServer.connected) {
            bleServiceFound.getCharacteristic(PaletteCharacteristic)
            .then(characteristic => {
                const data = new Uint8Array([value]);
                return characteristic.writeValue(data);
            })
            .then(() => {
                paletteValue.innerHTML = value;
                latestValueSent.innerHTML = value;
                console.log("Value written to Palette characteristic:", value);
            })
            .catch(error => {
                console.error("Error writing to the Palette characteristic: ", error);
            });
        } else {
            console.error ("Bluetooth is not connected. Cannot write to characteristic.")
            window.alert("Bluetooth is not connected. Cannot write to characteristic. \n Connect to BLE first!")
        }
    }

    function writeControlCharacteristic(value){
        if (bleServer && bleServer.connected) {
            bleServiceFound.getCharacteristic(ControlCharacteristic)
            .then(characteristic => {
//                console.log("Found Control characteristic: ", characteristic.uuid);
                const data = new Uint8Array([value]);
                return characteristic.writeValue(data);
            })
            .then(() => {
                latestValueSent.innerHTML = value;
                console.log("Value written to Control characteristic:", value);
            })
            .catch(error => {
                console.error("Error writing to the Control characteristic: ", error);
            });
        } else {
            console.error ("Bluetooth is not connected. Cannot write to characteristic.")
            window.alert("Bluetooth is not connected. Cannot write to characteristic. \n Connect to BLE first!")
        }
    }

    function disconnectDevice() {
        console.log("Disconnect Device.");
        bleServer.disconnect();
	    console.log("Device Disconnected");
        bleStateContainer.innerHTML = 'Disconnected';
        bleStateContainer.style.color = "#d13a30";
	}
  
    function getDateTime() {
        var currentdate = new Date();
        var day = ("00" + currentdate.getDate()).slice(-2); // Convert day to string and slice
        var month = ("00" + (currentdate.getMonth() + 1)).slice(-2);
        var year = currentdate.getFullYear();
        var hours = ("00" + currentdate.getHours()).slice(-2);
        var minutes = ("00" + currentdate.getMinutes()).slice(-2);
        var seconds = ("00" + currentdate.getSeconds()).slice(-2);
        var datetime = day + "/" + month + "/" + year + " at " + hours + ":" + minutes + ":" + seconds;
        return datetime;
    }










/*
document.addEventListener('DOMContentLoaded', () => {
  const myButton = document.getElementById('myButton');
  const mySlider = document.getElementById('mySlider');
  const sliderValueSpan = document.getElementById('sliderValue');
  const myCheckbox = document.getElementById('myCheckbox');
  const outputDiv = document.getElementById('output');

  // Button click event
  myButton.addEventListener('click', () => {
    outputDiv.textContent = 'Button clicked!';
  });

  // Slider value change event
  mySlider.addEventListener('input', () => {
    sliderValueSpan.textContent = mySlider.value;
    outputDiv.textContent = `Slider value changed to: ${mySlider.value}`;
  });

  // Checkbox state change event
  myCheckbox.addEventListener('change', () => {
    if (myCheckbox.checked) {
      outputDiv.textContent = 'Checkbox is checked!';
    } else {
      outputDiv.textContent = 'Checkbox is unchecked!';
    }
  });
});
*/