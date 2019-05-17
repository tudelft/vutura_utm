<template>
  <div id="app">
    <img alt="Vue logo" src="./assets/aed.png" width="200">
    <button v-promise-btn="{ promise: dataPromise }" @click="getData('param')">Request AED</button>
    <button v-promise-btn @click="startFlight">Start Flight</button>
    <div class="column" align="center">
      <drag-verify :width="killSlider.width"
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
         v-on:passcallback="killHandler"></drag-verify>
    </div>
  </div>
</template>

<script>
import Vue from 'vue'
import dragVerify from 'vue-drag-verify'
import VuePromiseBtn from 'vue-promise-btn'
import 'vue-promise-btn/dist/vue-promise-btn.css'

Vue.use(VuePromiseBtn)
export default {
  name: 'app',
  components: {
    dragVerify,
    VuePromiseBtn
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
        completedBg: "#ff0000",
      },
      dataPromise: null
    }
  },
  methods: {
    killHandler() {
      console.log("KILL SIGNAL");
    },
    dummyAsyncAction () {
      console.log("async");
      return new Promise((resolve, reject) => setTimeout(resolve, 2000))
    },
    isLoading() {
      console.log("isLoading");
    },
    getData() {
      console.log("getData");
      this.dataPromise = this.dummyAsyncAction();
      console.log("door");
    },
    startFlight() {
      console.log("Start flight");
    }
  }
}
</script>

<style>
#app {
  font-family: 'Avenir', Helvetica, Arial, sans-serif;
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
  -webkit-transition: background-color 0.1s linear, color 0.1s linear, width 0.2s ease;
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
