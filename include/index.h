#include "Arduino.h"
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">

    <head>
        <meta charset="UTF-8">
        <meta http-equiv="X-UA-Compatible"
            content="IE=edge">
        <meta name="viewport"
            content="width=device-width, initial-scale=1.0">
        <title>Litiere</title>

        <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css"
            rel="stylesheet"
            integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3"
            crossorigin="anonymous">

        <script type="text/javascript">
            var nettoyage = false;
            let vidange = false;
            function appelServeur(url, cFonction) {
                var xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function () {
                    if (this.readyState == 4 && this.status == 200) {
                        cFonction(this);
                    }
                };

                xhttp.open("GET", url, true);
                xhttp.send();
            }
            function stop(xhttp) {
                document.getElementById("stop").innerHTML = xhttp.responseText;
            }

            function onNettoyage(xhttp) {
                document.getElementById("nettoyage").innerHTML = "Nettoyage " + xhttp.responseText;
            }

            function onVidange(xhttp) {
                document.getElementById("vidange").innerHTML = "Vidange " + xhttp.responseText;
            }

        </script>
        <!-- Add Firebase products that you want to use -->
        <!--Firebase Libraries-->
        <script src="https://www.gstatic.com/firebasejs/7.15.5/firebase-app.js"></script>
        <script src="https://www.gstatic.com/firebasejs/7.15.5/firebase-auth.js"></script>
        <script src="https://www.gstatic.com/firebasejs/7.15.5/firebase-database.js"></script>
    </head>

    <body onload="init()">
        <div class="container d-flex flex-column mb-3 align-items-center justify-content-center">
            <h3>Gestion de la litière</h3>

            <div class="d-flex flex-column mb-3 align-items-center">
                <span>Status</span>
                <h1 id="status">0</h1>
                <div class="d-flex">
                    <div class="d-flex flex-column m-3 align-items-center">
                        <span>Sensor 1</span>
                        <h2 id="sensor1">0</h2>
                    </div>
                    <div class="d-flex flex-column m-3 align-items-center">
                        <span>Sensor 2</span>
                        <h2 id="sensor2">0</h2>
                    </div>
                </div>
            </div>

            <div class="d-flex flex-column mb-3 align-items-center">
                <label for="stop"
                    class="form-label">Arrêt de tout</label>
                <button class="btn btn-primary"
                    id="stop"
                    onclick="appelServeur('/stop', stop)">
                    Stop ALL
                </button>
            </div>
            <div class="d-flex flex-column mb-3 align-items-center">
                <label for="vidange"
                    class="form-label">Vidange des eaux sales</label>
                <div class=" d-flex justify-content-around">
                    <button class="btn btn-primary m-2"
                        onclick="appelServeur('/vidange?onOffVidange=0', onVidange)">
                        Vidange On
                    </button>
                    <button class="btn btn-primary  m-2"
                        onclick="appelServeur('/vidange?onOffVidange=1', onVidange)">
                        Vidange Off
                    </button>
                </div>
                <P id="vidange"></P>
            </div>
            <div class="d-flex flex-column mb-3 align-items-center">
                <label for="nettoyage"
                    class="form-label">Nettoyage de la litière</label>
                <div class=" d-flex justify-content-around">
                    <button class="btn btn-primary m-2"
                        onclick="appelServeur('/nettoyage?onOffNettoyage=0', onNettoyage)">
                        Nettoyage On
                    </button>
                    <button class="btn btn-primary  m-2"
                        onclick="appelServeur('/nettoyage?onOffNettoyage=1', onNettoyage)">
                        Nettoyage Off
                    </button>
                </div>
                <P id="nettoyage"></P>
            </div>
            <div class="d-flex flex-column mb-3 align-items-center">
                <h2 id="status">Reglage</h2>
                <div class="d-flex">
                    <div class="d-flex flex-column m-3 align-items-center">
                        <span>Sensor 1 max</span>
                        <span id="sensor1Maxvalue">0</span>
                        <input type="range"
                            id="sensor1Max"
                            min="100"
                            max="1000"
                            step="10"
                            oninput="sensor1MaxValueChange(this.value)">
                    </div>
                    <div class="d-flex flex-column m-3 align-items-center">
                        <span>Sensor 2 max</span>
                        <span id="sensor2Maxvalue">0</span>
                        <input type="range"
                            id="sensor2Max"
                            min="100"
                            max="1000"
                            step="10"
                            oninput="sensor2MaxValueChange(this.value)">
                    </div>
                </div>
            </div>
            <div class="d-flex flex-column m-3 align-items-center">
                <span>Temps de nettoyage</span>
                <span id="tempMaxvalue">0</span>
                <input type="range"
                    id="tempMax"
                    min="10"
                    max="240"
                    step="10"
                    oninput="tempMaxValueChange(this.value)">
            </div>
            <div class="d-flex">
                <span class="p-1">Niveau de l'eau: </span>
            </div>
            <div #waterSensor
                class="alert alert-danger"
                id="waterSensor"
                role="alert">
                Niveau de l'eau Ok
            </div>
            <table class="table">
                <thead class="thead-dark">
                    <tr>
                        <th scope="col">#</th>
                        <th scope="col">Date</th>
                        <th scope="col">Time</th>
                        <th scope="col">Sensor 1</th>
                        <th scope="col">Sensor 2</th>
                        <th scope="col">Water</th>
                    </tr>
                </thead>
                <tbody id="tBody1">

                </tbody>
            </table>
        </div>
        </div>

        <script>
            var webSocket;
            function init() {
                initFireBase();

                webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
                webSocket.onmessage = function (event) {
                    var data = JSON.parse(event.data);
                    document.getElementById("status").innerHTML = data.status
                    document.getElementById("sensor1").innerHTML = data.lidar[0]
                    document.getElementById("sensor2").innerHTML = data.lidar[1]
                    document.getElementById("sensor1Maxvalue").innerHTML = data.lidarDistanceMaxSensor1
                    document.getElementById("sensor2Maxvalue").innerHTML = data.lidarDistanceMaxSensor2
                    document.getElementById("tempMaxvalue").innerHTML = data.duringWaterOn
                    document.getElementById("sensor1Max").value = data.lidarDistanceMaxSensor1
                    document.getElementById("sensor2Max").value = data.lidarDistanceMaxSensor2
                    document.getElementById("tempMax").value = data.duringWaterOn
                    if (data.waterSensor) {
                        document.getElementById("waterSensor").innerHTML = "Niveau d'eau Ok"
                        document.getElementById("waterSensor").classList.replace('alert-danger', 'alert-success')
                    } else {
                        document.getElementById("waterSensor").innerHTML = "Niveau d'eau BAS"
                        document.getElementById("waterSensor").classList.replace('alert-success', 'alert-danger')
                    }
                    //console.table(data)
                }

            }
            function sensor1MaxValueChange(newvalue) {
                document.getElementById("sensor1Maxvalue").innerHTML = newvalue;
                webSocket.send(JSON.stringify({ sensor1Maxvalue: newvalue }))
            }

            function sensor2MaxValueChange(newvalue) {
                document.getElementById("sensor2Maxvalue").innerHTML = newvalue;
                webSocket.send(JSON.stringify({ sensor2Maxvalue: newvalue }))
            }

            function tempMaxValueChange(newvalue) {
                document.getElementById("tempMaxvalue").innerHTML = newvalue;
                webSocket.send(JSON.stringify({ tempMaxvalue: newvalue }))
            }

            function initFireBase() {
                const firebaseConfig = {
                    apiKey: "AIzaSyCNp5y5Qhs9n6SXI06CGI8fniae_FDmmKY",
                    authDomain: "litiere.firebaseapp.com",
                    databaseURL: "https://litiere-default-rtdb.europe-west1.firebasedatabase.app",
                    projectId: "litiere",
                    storageBucket: "litiere.appspot.com",
                    messagingSenderId: "258666584011",
                    appId: "1:258666584011:web:52bcfc41daa1f26691884e"
                };

                // Initialize Firebase

                firebase.initializeApp(firebaseConfig);
                console.log('initializeApp')
                var itemsRef = firebase.database().ref('litiere').child("json");
                initData(itemsRef);
            }

            function initData(itemsRef) {
                console.log(itemsRef)
                itemsRef.once('value', function (snapshot) {
                    snapshot.forEach(function (item_snapshot) {
                        let date = item_snapshot.val().date;
                        let sensor1Value = item_snapshot.val().sensor1Value;
                        let sensor2Value = item_snapshot.val().sensor2Value;
                        let hasWater = item_snapshot.val().hasWater;
                        let time = item_snapshot.val().time;
                        addIntable(date, time, sensor1Value, sensor2Value, hasWater);
                    });
                });

            }

            let index = 0;
            function addIntable(date, time, sensor1Value, sensor2Value, hasWater) {
                let tBody = document.getElementById("tBody1")
                let trow = document.createElement("tr");
                let td1 = document.createElement("td");
                let td2 = document.createElement("td");
                let td3 = document.createElement("td");
                let td4 = document.createElement("td");
                let td5 = document.createElement("td");
                let td6 = document.createElement("td");
                td1.innerHTML = ++index;
                td2.innerHTML = date;
                td3.innerHTML = time;
                td4.innerHTML = sensor1Value;
                td5.innerHTML = sensor2Value;
                td6.innerHTML = hasWater;

                trow.appendChild(td1);
                trow.appendChild(td2);
                trow.appendChild(td3);
                trow.appendChild(td4);
                trow.appendChild(td5);
                trow.appendChild(td6);
                tBody.appendChild(trow);
            }

        </script>

        <script src="https://code.jquery.com/jquery-3.2.1.slim.min.js"
            integrity="sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN"
            crossorigin="anonymous"></script>
        <script src="https://cdn.jsdelivr.net/npm/popper.js@1.12.9/dist/umd/popper.min.js"
            integrity="sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q"
            crossorigin="anonymous"></script>
        <script src="https://cdn.jsdelivr.net/npm/bootstrap@4.0.0/dist/js/bootstrap.min.js"
            integrity="sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl"
            crossorigin="anonymous"></script>

    </body>

</html>


)=====";