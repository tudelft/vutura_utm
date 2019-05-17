<template>
  <div id="app">
    <img alt="Vue logo" src="./assets/logo.png">
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
      </drag-verify>
    </div>
  </div>
</template>

<script>
import dragVerify from 'vue-drag-verify'
export default {
  name: 'app',
  components: {
    dragVerify
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
      }
    }
  },
  methods: {
    killHandler() {
      console.log("KILL SIGNAL");
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
</style>
