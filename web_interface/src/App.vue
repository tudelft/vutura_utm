<template>
  <div id="app">
    <div class="row">
      <img alt="Vue logo" src="./assets/aed.png" width="200">
      <br>
      <br>
    </div>
    <div class="column">
      <button
        id="requestFlightButton"
        v-promise-btn="{ promise: requestFlightPromise }"
        @click="requestFlight"
      >Request Flight</button>
      <button
        id="startFlightButton"
        v-promise-btn="{ promise: startFlightPromise }"
        @click="startFlight"
      >Start Flight</button>
      <button
        id="endFlightButton"
        v-promise-btn="{ promise: endFlightPromise }"
        @click="endFlight"
      >End Flight</button>
      <br>
      <br>
    </div>
    <div class="column" align="center">
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
  </div>
</template>

<script>
import Vue from "vue";
import dragVerify from "vue-drag-verify";
import VuePromiseBtn from "vue-promise-btn";
import "vue-promise-btn/dist/vue-promise-btn.css";

Vue.use(VuePromiseBtn);
export default {
  name: "app",
  components: {
    dragVerify,
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
      requestFlightPromise: null,
      startFlightPromise: null,
      endFlightPromise: null,
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
    updateUi(state) {
      if (state == "init") {
        requestFlightButton.disabled = true;
        startFlightButton.disabled = true;
        endFlightButton.disabled = true;
      } else if (state == "logged in") {
        requestFlightButton.disabled = false;
        startFlightButton.disabled = true;
        endFlightButton.disabled = true;
      } else if (state == "flight requested") {
        requestFlightButton.disabled = true;
        startFlightButton.disabled = true;
        endFlightButton.disabled = true;
      } else if (state == "flight authorized") {
        requestFlightButton.disabled = true;
        startFlightButton.disabled = false;
        endFlightButton.disabled = true;
      } else if (state == "flight started") {
        requestFlightButton.disabled = true;
        startFlightButton.disabled = true;
        endFlightButton.disabled = false;
      } else {
        console.log("Halllo");
      }
    }
  },
  sockets: {
    utmsp_response: function(data) {
      console.log("response", data.toString());
      this.promiseResolve();
      this.updateUi(data.toString());
    },
    state_update: function(data) {
      console.log("state update", data.toString());
      this.updateUi(data.toString());
    }
  }
};
</script>

<style>
#app {
  font-family: "Avenir", Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
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
  margin: 0;
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
</style>
