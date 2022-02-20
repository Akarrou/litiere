#include "Arduino.h"
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">

<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Litiere</title>

    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet"
        integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">

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
        <div class="col-4">
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
                <label for="stop" class="form-label">Arrêt de tout</label>
                <button class="btn btn-primary" id="stop" onclick="appelServeur('/stop', stop)">
                    Stop ALL
                </button>
            </div>
            <div class="d-flex flex-column mb-3 align-items-center">
                <label for="vidange" class="form-label">Vidange des eaux sales</label>
                <div class=" d-flex justify-content-around">
                    <button class="btn btn-primary m-2" onclick="appelServeur('/vidange?onOffVidange=1', onVidange)">
                        Vidange On
                    </button>
                    <button class="btn btn-primary  m-2" onclick="appelServeur('/vidange?onOffVidange=O', onVidange)">
                        Vidange Off
                    </button>
                </div>
                <P id="vidange"></P>
            </div>
            <div class="d-flex flex-column mb-3 align-items-center">
                <label for="nettoyage" class="form-label">Nettoyage de la litière</label>
                <div class=" d-flex justify-content-around">
                    <button class="btn btn-primary m-2"
                        onclick="appelServeur('/nettoyage?onOffNettoyage=1', onNettoyage)">
                        Nettoyage On
                    </button>
                    <button class="btn btn-primary  m-2"
                        onclick="appelServeur('/nettoyage?onOffNettoyage=O', onNettoyage)">
                        Nettoyage Off
                    </button>
                </div>
                <P id="nettoyage"></P>
            </div>
            <div class="d-flex flex-column mb-3 align-items-center">
                <h2 id="status">Reglage</h2>
                <div class="d-flex">
                    <div class="d-flex flex-column m-3 align-items-center">
                        <span>Sensor max</span>
                        <span id="sensorMaxvalue">0</span>
                        <input type="range" id="sensorMax" min="100" max="1000" step="10"
                            oninput="sensorMaxvalueChange(this.value)">
                    </div>
                    <div class="d-flex flex-column m-3 align-items-center">
                        <span>Temps de nettoyage</span>
                        <span id="tempMaxvalue">0</span>
                        <input type="range" id="tempMax" min="10" max="240" step="30"
                            oninput="tempMaxvalueChange(this.value)">
                    </div>
                </div>
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
                document.getElementById("sensorMaxvalue").innerHTML = data.lidarDistanceMax
                document.getElementById("tempMaxvalue").innerHTML = data.duringWaterOn
                document.getElementById("sensorMax").value = data.lidarDistanceMax
                document.getElementById("tempMax").value = data.duringWaterOn
                console.log(data)
            }
        }
        function sensorMaxvalueChange(newvalue) {
            document.getElementById("sensorMaxvalue").innerHTML = newvalue;
            webSocket.send(JSON.stringify({ sensorMaxvalue: newvalue }))
        }

        function tempMaxvalueChange(newvalue) {
            document.getElementById("tempMaxvalue").innerHTML = newvalue;
            webSocket.send(JSON.stringify({ tempMaxvalue: newvalue }))
        }
    </script>

</body>

</html>
)=====";