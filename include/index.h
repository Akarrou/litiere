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

        function DisplayChange(newvalue) {
            document.getElementById(
                "value").innerHTML = newvalue;
            appelServeur('/duringWater?setDuringWaterOn=' + newvalue, onNettoyage);
        }
    </script>
</head>

<body>
    <div class="container d-flex flex-column mb-3 align-items-center justify-content-center">
        <h3>Gestion de la litière</h3>
        <div class="col-4">
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
                <label for="duringWaterOn" class="form-label">Temps de nettoyage</label>
                <div class=" d-flex justify-content-around">
                    <input type="range" class="form-range" oninput="DisplayChange(this.value)" min="5" max="120"
                        step="1" id="duringWaterOnInput">
                </div>
                <span id="value"></span>
            </div>
        </div>
    </div>

</body>

</html>
)=====";