// Firebase
const firebaseConfig = {
    apiKey: "AIzaSyA6e3mTRJTL2qnhaAE9EM--wNlKt5RYNUM",
    authDomain: "test2-f5585.firebaseapp.com",
    databaseURL: "https://test2-f5585-default-rtdb.asia-southeast1.firebasedatabase.app/",
    projectId: "test2-f5585",
    storageBucket: "test2-f5585.firebasestorage.app",
    messagingSenderId: "73192245697",
    appId: "1:73192245697:web:44f14ec5b566b538004740",
    measurementId: "G-E8NNP1286V"
};

firebase.initializeApp(firebaseConfig);
firebase.analytics();
const database = firebase.database();

var aqi = document.getElementById("aqi");
var pm25 = document.getElementById("pm25");
var pm10 = document.getElementById("pm10");
var co2 = document.getElementById("co2");
var tvoc = document.getElementById("tvoc");
var temp = document.getElementById("temp");
var humid = document.getElementById("humid");

var dbRef1 = firebase.database().ref().child("/Sensor/AQI_h");
var dbRef3 = firebase.database().ref().child("/Sensor/PM25");
var dbRef4 = firebase.database().ref().child("/Sensor/PM10");
var dbRef5 = firebase.database().ref().child("/Sensor/CO2");
var dbRef6 = firebase.database().ref().child("/Sensor/TVOC");
var dbRef7 = firebase.database().ref().child("/Sensor/Temp");
var dbRef8 = firebase.database().ref().child("/Sensor/Humid");

// Biến để lưu trữ giá trị
var aqiValue, pm1Value, pm25Value, pm10Value, co2Value, tvocValue, tempValue, humidValue;

// Lấy giá trị từ Firebase để xử lý và cập nhập giá trị lên web
dbRef1.on("value", (snap) => {
    aqi.innerHTML = snap.val();
    aqiValue = snap.val();
    document.getElementById("gaugeFillAQI").style.transform = `rotate(${aqiValue * 0.36
        }deg)`;
    if (aqiValue <= 50) {
        document.getElementById("gaugeFillAQI").style.backgroundColor = "#00e400";
    } else if (aqiValue >= 51 && aqiValue <= 100) {
        document.getElementById("gaugeFillAQI").style.backgroundColor = "#FFFF00";
    } else if (aqiValue >= 101 && aqiValue <= 150) {
        document.getElementById("gaugeFillAQI").style.backgroundColor = "#FF7E00";
    } else if (aqiValue >= 151 && aqiValue <= 200) {
        document.getElementById("gaugeFillAQI").style.backgroundColor = "#FF0000";
    } else if (aqiValue >= 201 && aqiValue <= 300) {
        document.getElementById("gaugeFillAQI").style.backgroundColor = "#8F3F97";
    } else {
        document.getElementById("gaugeFillAQI").style.backgroundColor = "#7E0023";
    }
    console.log("Giá trị AQI:", aqiValue);
});

dbRef3.on("value", (snap) => {
    pm25.innerHTML = snap.val();
    pm25Value = snap.val();
    document.getElementById("gaugeFillPM25").style.transform = `rotate(${pm25Value * 3
        }deg)`;
    if (pm25Value <= 12) {
        document.getElementById("gaugeFillPM25").style.backgroundColor = "#00e400";
    } else if (pm25Value >= 13 && pm25Value <= 35) {
        document.getElementById("gaugeFillPM25").style.backgroundColor = "#FFFF00";
    } else if (pm25Value >= 36 && pm25Value <= 55) {
        document.getElementById("gaugeFillPM25").style.backgroundColor = "#FF7E00";
    } else {
        document.getElementById("gaugeFillPM25").style.backgroundColor = "#FF0000";
    }
    console.log("Giá trị PM25:", pm25Value);
});

dbRef4.on("value", (snap) => {
    pm10.innerHTML = snap.val();
    pm10Value = snap.val();
    document.getElementById("gaugeFillPM10").style.transform = `rotate(${pm10Value * 0.6923076923076923
        }deg)`;
    if (pm10Value <= 54) {
        document.getElementById("gaugeFillPM10").style.backgroundColor = "#00e400";
    } else if (pm10Value >= 55 && pm10Value <= 154) {
        document.getElementById("gaugeFillPM10").style.backgroundColor = "#FFFF00";
    } else if (pm10Value >= 155 && pm10Value <= 254) {
        document.getElementById("gaugeFillPM10").style.backgroundColor = "#FF7E00";
    } else {
        document.getElementById("gaugeFillPM10").style.backgroundColor = "#FF0000";
    }
    console.log("Giá trị PM10:", pm10Value);
});

dbRef5.on("value", (snap) => {
    co2.innerHTML = snap.val();
    co2Value = snap.val();
    document.getElementById("gaugeFillCO2").style.transform = `rotate(${co2Value * 0.03
        }deg)`;
    if (co2Value <= 999) {
        document.getElementById("gaugeFillCO2").style.backgroundColor = "#00e400";
    } else if (co2Value >= 1000 && co2Value <= 2000) {
        document.getElementById("gaugeFillCO2").style.backgroundColor = "#FFFF00";
    } else if (co2Value >= 2001 && co2Value <= 5000) {
        document.getElementById("gaugeFillCO2").style.backgroundColor = "#FF7E00";
    } else {
        document.getElementById("gaugeFillCO2").style.backgroundColor = "#FF0000";
    }
    console.log("Giá trị CO2:", co2Value);
});

dbRef6.on("value", (snap) => {
    tvoc.innerHTML = snap.val();
    tvocValue = snap.val();
    document.getElementById("gaugeFillTVOC").style.transform = `rotate(${tvocValue * 0.0327272727272727
        }deg)`;
    if (tvocValue <= 220) {
        document.getElementById("gaugeFillTVOC").style.backgroundColor = "#00e400";
    } else if (tvocValue >= 221 && tvocValue <= 660) {
        document.getElementById("gaugeFillTVOC").style.backgroundColor = "#FFFF00";
    } else if (tvocValue >= 661 && tvocValue <= 1430) {
        document.getElementById("gaugeFillTVOC").style.backgroundColor = "#FF7E00";
    } else if (tvocValue >= 1431 && tvocValue <= 2200) {
        document.getElementById("gaugeFillTVOC").style.backgroundColor = "#FF0000";
    } else if (tvocValue >= 2201 && tvocValue <= 3300) {
        document.getElementById("gaugeFillTVOC").style.backgroundColor = "#8F3F97";
    } else {
        document.getElementById("gaugeFillTVOC").style.backgroundColor = "#7E0023";
    }
    console.log("Giá trị TVOC:", tvocValue);
});

dbRef7.on("value", (snap) => {
    temp.innerHTML = snap.val();
    tempValue = snap.val();
    document.getElementById("gaugeFillTemp").style.transform = `rotate(${tempValue * 3.6
        }deg)`;
    if (tempValue < 30) {
        document.getElementById("gaugeFillTemp").style.backgroundColor = "#00e400";
    } else if (tempValue >= 30 && tempValue < 35) {
        document.getElementById("gaugeFillTemp").style.backgroundColor = "#FFFF00";
    } else if (tempValue >= 35 && tempValue < 40) {
        document.getElementById("gaugeFillTemp").style.backgroundColor = "#FF7E00";
    } else if (tempValue >= 40 && tempValue < 45) {
        document.getElementById("gaugeFillTemp").style.backgroundColor = "#FF0000";
    } else {
        document.getElementById("gaugeFillTemp").style.backgroundColor = "#8F3F97";
    }
    console.log("Giá trị Temp:", tempValue);
});

dbRef8.on("value", (snap) => {
    humid.innerHTML = snap.val();
    humidValue = snap.val();
    document.getElementById("gaugeFillHumid").style.transform = `rotate(${humidValue * 1.8
        }deg)`;
    if (humidValue <= 80) {
        document.getElementById("gaugeFillHumid").style.backgroundColor = "#00e400";
    } else if (humidValue > 80 && humidValue <= 85) {
        document.getElementById("gaugeFillHumid").style.backgroundColor = "#FFFF00";
    } else if (humidValue > 85 && humidValue <= 90) {
        document.getElementById("gaugeFillHumid").style.backgroundColor = "#FF7E00";
    } else if (humidValue > 90 && humidValue <= 95) {
        document.getElementById("gaugeFillHumid").style.backgroundColor = "#FF0000";
    } else {
        document.getElementById("gaugeFillHumid").style.backgroundColor = "#8F3F97";
    }
    console.log("Giá trị Humid:", humidValue);
});

// Cập nhập giá trị button về Firebase
controlTopOff.onclick = function () {
    document.getElementById('controlTopOnOff').style.left = '0'
    document.getElementById('controlTopOnOffImg').src = "./assets/icons/plug-off.svg"
    database.ref("/Control").update({
        OnOff: 0,
    });
}

controlTopOn.onclick = function () {
    document.getElementById('controlTopOnOff').style.left = '42px'
    document.getElementById('controlTopOnOffImg').src = "./assets/icons/plug-on.svg"
    database.ref("/Control").update({
        OnOff: 1,
    });
}

// controlTopChildLockOff.onclick = function () {
//     document.getElementById('controlTopChildLock').style.left = '0'
//     document.getElementById('controlTopChildLockImg').src = "./assets/icons/baby-solid.svg"
//     database.ref("/Control").update({
//         Lock: 0,
//     });
// }

// controlTopChildLockOn.onclick = function () {
//     document.getElementById('controlTopChildLock').style.left = '42px'
//     document.getElementById('controlTopChildLockImg').src = "./assets/icons/baby-solid-on.svg"
//     database.ref("/Control").update({
//         Lock: 1,
//     });
// }

btnOff.onclick = function () {
    document.getElementById('btnOne').style.left = '0'
    database.ref("/Control").update({
        Ion: 0,
    });
}

btnOn.onclick = function () {
    document.getElementById('btnOne').style.left = '42px'
    database.ref("/Control").update({
        Ion: 1,
    });
}

// Cập nhập giá trị thanh trượt và chế độ update Firebase
var myRange = document.getElementById("myRange");
var updateRange = document.getElementById("updateRange");

myRange.oninput = function () {
    updateRange.innerHTML = this.value;
    database.ref("/Control").update({
        Speed: Number(this.value),
    });
};

const radioInput1 = document.getElementById("auto");
const radioInput2 = document.getElementById("normal");
// const radioInput3 = document.getElementById("sleep");

radioInput1.addEventListener("change", () => {
    if (radioInput1.checked) {
        const selectedValue = radioInput1.value;
        console.log(selectedValue)
        database.ref("/Control").update({
            Mode: Number(selectedValue),
        });
    }
});

radioInput2.addEventListener("change", () => {
    if (radioInput2.checked) {
        const selectedValue = radioInput2.value;
        console.log(selectedValue)
        database.ref("/Control").update({
            Mode: Number(selectedValue),
        });
    }
});

// radioInput3.addEventListener("change", () => {
//     if (radioInput3.checked) {
//         const selectedValue = radioInput3.value;
//         console.log(selectedValue)
//         database.ref("/Control").update({
//             Mode: Number(selectedValue),
//         });
//     }
// });


// Tự động update hình ảnh
database.ref("/Control/OnOff").on("value", function (snapshot) {
    if (snapshot.exists()) {
        if (snapshot.val() == 1) {
            document.getElementById('controlTopOnOff').style.left = '42px'
            document.getElementById('controlTopOnOffImg').src = "./assets/icons/plug-on.svg"
        }
        else {
            document.getElementById('controlTopOnOff').style.left = '0'
            document.getElementById('controlTopOnOffImg').src = "./assets/icons/plug-off.svg"
        }
    } else console.log("No data available!");
});

// database.ref("/Control/Lock").on("value", function (snapshot) {
//     if (snapshot.exists()) {
//         if (snapshot.val() == 1) {
//             document.getElementById('controlTopChildLock').style.left = '42px'
//             document.getElementById('controlTopChildLockImg').src = "./assets/icons/baby-solid-on.svg"
//         }
//         else {
//             document.getElementById('controlTopChildLock').style.left = '0'
//             document.getElementById('controlTopChildLockImg').src = "./assets/icons/baby-solid.svg"
//         }
//     } else console.log("No data available!");
// });

database.ref("/Control/Ion").on("value", function (snapshot) {
    if (snapshot.exists()) {
        if (snapshot.val() == 1) {
            document.getElementById('btnOne').style.left = '42px'
        }
        else {
            document.getElementById('btnOne').style.left = '0'
        }
    } else console.log("No data available!");   
});

database.ref("/Control/Speed").on("value", function (snapshot) {
    if (snapshot.exists()) {
        var speed = snapshot.val();
        if (speed > 0) {
            myRange.value = speed;
            updateRange.innerHTML = speed;
        }
        else {
            myRange.value = speed;
            updateRange.innerHTML = speed;
        }
    } else console.log("No data available!");
});

database.ref("/Control/Mode").on("value", function (snapshot) {
    if (snapshot.exists()) {
        var mode = snapshot.val();
        if (mode == 1) {
            document.getElementById("auto").checked = true;
        } else if (mode == 2) {
            document.getElementById("normal").checked = true;
        } 
    } else console.log("No data available!");
});

// Lấy data lần đầu khi truy cập website
database
    .ref("/Control")
    .get()
    .then((snapshot) => {
        if (snapshot.exists()) console.log(snapshot.val());
        else console.log("No data available!");
    });

database
    .ref("/Sensor")
    .get()
    .then((snapshot) => {
        if (snapshot.exists()) console.log(snapshot.val());
        else console.log("No data available!");
    });