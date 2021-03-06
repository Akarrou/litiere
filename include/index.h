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

        </div>
        </div>

        <script>
            var webSocket;
            function init() {
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