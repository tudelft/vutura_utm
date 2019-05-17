import Vue from 'vue'
import App from './App.vue'
import VueSocketIO from 'vue-socket.io'

Vue.config.productionTip = false

var origin = location.origin.split(':')
var target = origin[0] + ':' + origin[1] + ':3000'
Vue.use(new VueSocketIO({
	debug: true,
	connection: target
}))

new Vue({
	render: h => h(App),
	sockets: {
		connect: function() {
			console.log("socket connected")
			this.isConnected = true
		},
		disconnect: function() {
			console.log("socket disconnected")
			this.isConnected = false
		}
	}
}).$mount('#app')
