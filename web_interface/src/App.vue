<template>
  <div id="main">
    <div class="row">
      <mapbox
        access-token="pk.eyJ1IjoibWFwdHVyZSIsImEiOiJjanNobzhlZjkxeHlvM3lvNjFsMW93c2pnIn0.NktfcdyQZcfvhxaWCrh2qg"
        :map-options="{
        style: 'mapbox://styles/mapbox/satellite-v9',
        center: [4.4179034, 52.1686828],
        zoom: 16,
        fadeDuration: 0
      }"
        :geolocate-control="{
        show: true,
        position: 'bottom-right'
      }"
        :scale-control="{
        show: true,
        position: 'bottom-right'
      }"
        @map-load="mapLoaded"
      ></mapbox>
      <div class="map-overlay top">
        <div class="map-overlay-inner">
          <label>Height: {{ drone.height }} m</label>
        </div>
      </div>
    </div>
    <div class="row" style="margin-bottom:100px; margin-top:40px">
      <div style="float:left; margin-right:40px">
        <img alt="Vue logo" src="./assets/aed.png" width="200">
      </div>
      <div>
        <button
          id="requestAedButton"
          disabled="disabled"
          v-promise-btn="{ promise: requestAedPromise }"
          @click="requestAed"
        >Request AED</button>
        <button
          id="abortAedButton"
          disabled="disabled"
          style="margin-left:20px"
          v-promise-btn="{ promise: abortAedPromise }"
          @click="abortAed"
        >Abort request</button>
        <p class="state_text">State: {{ utm_state }}</p>
      </div>
    </div>
    <div class="row" align="center">
      <drag-verify
        :width="killSlider.width"
        :height="killSlider.height"
        :text="killSlider.text"
        :success-text="killSlider.successText"
        :background="killSlider.background"
        :progress-bar-bg="killSlider.progressBarBg"
        :completed-bg="killSlider.completedBg"
        :handler-bg="killSlider.handlerBg"
        :handler-icon="killSlider.handlerIcon"
        :text-size="killSlider.textSize"
        :success-icon="killSlider.successIcon"
        v-on:passcallback="killHandler"
      ></drag-verify>
    </div>
    <div class="row" style="margin-top: 100px"></div>
    <div class="column">
      <button id="mavPause" @click="mavPause">Pause</button>
      <button id="mavContinue" @click="mavContinue">Continue</button>
      <button id="mavLandHere" @click="mavLandHere">Land here</button>
    </div>
    <div class="row" style="margin-top: 100px"></div>
    <div class="column">
      <button
        id="requestFlightButton"
        disabled="disabled"
        v-promise-btn="{ promise: requestFlightPromise }"
        @click="requestFlight"
      >Request Flight</button>
      <button
        id="startFlightButton"
        style="margin-left:20px"
        disabled="disabled"
        v-promise-btn="{ promise: startFlightPromise }"
        @click="startFlight"
      >Start Flight</button>
      <button
        id="endFlightButton"
        style="margin-left:20px"
        disabled="disabled"
        v-promise-btn="{ promise: endFlightPromise }"
        @click="endFlight"
      >End Flight</button>
    </div>
  </div>
</template>

<script>
import Vue from "vue";
import dragVerify from "vue-drag-verify";
import VuePromiseBtn from "vue-promise-btn";
import "vue-promise-btn/dist/vue-promise-btn.css";
import Mapbox from "mapbox-gl-vue";

Vue.use(VuePromiseBtn);
export default {
  name: "app",
  components: {
    dragVerify,
    mapbox: Mapbox
  },
  data() {
    return {
      killSlider: {
        width: 900,
        height: 80,
        text: "KILL - PARACHUTE DEPLOY",
        successText: "KILL SIGNAL SENT",
        textSize: "40px",
        progressBarBg: "#ff0000",
        completedBg: "#ff0000"
      },
      map: null,
      drone: {
        height: "0",
        lat: 52.1686828,
        lon: 4.4179034,
        hdg: 70
      },
      utm_state: "Not connected",
      requestFlightPromise: null,
      startFlightPromise: null,
      endFlightPromise: null,
      requestAedPromise: null,
      abortAedPromise: null,
      promiseResolve: null,
      promiseReject: null
    };
  },
  methods: {
    killHandler() {
      console.log("KILL SIGNAL");
    },
    dummyAsyncAction() {
      return new Promise(
        function(resolve, reject) {
          this.promiseResolve = resolve;
          this.promiseReject = reject;
        }.bind(this)
      );
    },
    isLoading() {
      console.log("isLoading");
    },
    requestFlight() {
      this.updateUi("init");
      console.log("request_flight");
      this.$socket.emit("request_flight");
      this.requestFlightPromise = this.dummyAsyncAction();
    },
    startFlight() {
      this.updateUi("init");
      this.$socket.emit("start_flight");
      console.log("Start flight");
      this.startFlightPromise = this.dummyAsyncAction();
    },
    endFlight() {
      this.updateUi("init");
      this.$socket.emit("end_flight");
      console.log("End flight");
      this.endFlightPromise = this.dummyAsyncAction();
    },
    requestAed() {
      this.updateUi("init");
      this.$socket.emit("request_autoflight");
      console.log("Start autoflight");
      this.requestAedPromise = this.dummyAsyncAction();
    },
    abortAed() {
      this.updateUi("init");
      this.$socket.emit("abort_autoflight");
      console.log("Abort autoflight");
      this.abortAedPromise = this.dummyAsyncAction();
    },
    mavPause() {
      console.log("pause");
      this.$socket.emit("pause");
    },
    mavContinue() {
      console.log("continue");
      this.$socket.emit("continue");
    },
    mavLandHere() {
      console.log("land here");
      this.$socket.emit("land here");
    },
    updateState(state) {
      this.utm_state = state;
      this.updateUi(state);
    },
    updateUi(state) {
      if (state == "init") {
        requestAedButton.disabled = true;
        abortAedButton.disabled = true;
        requestFlightButton.disabled = true;
        startFlightButton.disabled = true;
        endFlightButton.disabled = true;
      } else if (state == "logged in") {
        requestAedButton.disabled = false;
        abortAedButton.disabled = true;
        requestFlightButton.disabled = false;
        startFlightButton.disabled = true;
        endFlightButton.disabled = true;
      } else if (state == "flight requested") {
        requestAedButton.disabled = true;
        abortAedButton.disabled = false;
        requestFlightButton.disabled = true;
        startFlightButton.disabled = true;
        endFlightButton.disabled = true;
      } else if (state == "flight authorized") {
        requestAedButton.disabled = true;
        abortAedButton.disabled = false;
        requestFlightButton.disabled = true;
        startFlightButton.disabled = false;
        endFlightButton.disabled = true;
      } else if (state == "flight started") {
        requestAedButton.disabled = true;
        abortAedButton.disabled = false;
        requestFlightButton.disabled = true;
        startFlightButton.disabled = true;
        endFlightButton.disabled = false;
      } else if (state == "armed") {
        requestAedButton.disabled = true;
        abortAedButton.disabled = true;
        requestFlightButton.disabled = true;
        startFlightButton.disabled = true;
        endFlightButton.disabled = false;
      } else {
        console.log("Halllo");
      }
    },
    mapLoaded(map) {
      map.loadImage(require("./assets/aed.png"), function(error, image) {
        if (error) throw error;
        map.addImage("drone", image);
        map.addLayer({
          id: "drone_icon",
          type: "symbol",
          source: {
            type: "geojson",
            data: {
              type: "FeatureCollection",
              features: [
                {
                  type: "Feature",
                  geometry: {
                    type: "Point",
                    coordinates: [4.4129034, 52.1681828]
                  }
                }
              ]
            }
          },
          layout: {
            "icon-image": "drone",
            "icon-size": 0.07,
            "icon-rotate": 70
          }
        });
      });

      this.map = map;
    },
    updateAedPosition() {
      if (this.map) {
        this.map.getSource("drone_icon").setData({
          type: "Point",
          coordinates: [this.drone.lon, this.drone.lat]
        });
        this.map.setLayoutProperty("drone_icon", "icon-rotate", this.drone.hdg);
      }
    }
  },
  sockets: {
    utmsp_response: function(data) {
      console.log("response", data.toString());
      this.promiseResolve();
      this.updateState(data.toString());
    },
    state_update: function(data) {
      console.log("state update", data.toString());
      this.updateState(data.toString());
    },
    position_update: function(data) {
      var gpos = JSON.parse(data);
      this.drone.lat = gpos.lat * 1e-7;
      this.drone.lon = gpos.lon * 1e-7;
      this.drone.hdg = gpos.hdg * 1e-2;
      this.drone.height = (gpos.height * 1e-3).toFixed(2).toString();

      this.updateAedPosition();
    }
  }
};
</script>

<style>
#main {
  font-family: "Avenir", Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  /* text-align: center; */
  color: #2c3e50;
  /* margin-top: 60px; */
}

.state_text {
  font: 34px/40px Arial, Helverica, sans-serif;
}

button,
input[type="submit"] {
  display: inline-block;
  vertical-align: middle;
  background-color: #3b98ab;
  border: 1px solid #3b98ab;
  border-radius: 14px;
  padding: 18px 22px;
  font: 34px/40px Arial, Helverica, sans-serif;
  color: #fff;
  margin: 0px;
  text-decoration: none;
  text-align: center;
  cursor: pointer;
  -webkit-transition: background-color 0.1s linear, color 0.1s linear,
    width 0.2s ease;
  transition: background-color 0.1s linear, color 0.1s linear, width 0.2s ease;
}

button[disabled],
input[type="submit"][disabled] {
  background: #a0d4de;
  border-color: #a0d4de;
  cursor: default;
}

button .promise-btn__spinner-wrapper {
  display: inline-block;
  margin: -8px 0;
}

button.hide-loader .hidden {
  display: none;
}

.site-footer {
  display: flex;
  justify-content: space-between;
}

#map {
  width: 100%;
  height: 600px;
}

.map-overlay {
  font: bold 20px/20px "Helvetica Neue", Arial, Helvetica, sans-serif;
  position: absolute;
  width: 200px;
  top: 10px;
  left: 10px;
  padding: 10px;
}

.map-overlay .map-overlay-inner {
  background-color: #fff;
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.2);
  border-radius: 3px;
  padding: 10px;
  margin-bottom: 10px;
}

.map-overlay label {
  display: block;
  /* margin: 0 0 10px; */
}
</style>
