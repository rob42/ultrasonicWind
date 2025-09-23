// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create AWA Gauge
var gaugeAwa = new RadialGauge({
  renderTo: 'gauge-awa',
  width: 300,
  height: 300,
  units: "AWA",
  minValue: 0,
  maxValue: 360,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueBox: false,
  valueInt: 3,
  valueDec: 0,
  ticksAngle: 360,
  startAngle: 180,
  useMinPath: true,
  majorTicks: [
      "0",
      "30",
      "60",
      "90",
      "120",
      "150",
      "180",
      "210",
      "240",
      "270",
      "300",
      "330",
      ""
  ],
  minorTicks: 3,
  strokeTicks: true,
  highlights: [
  	
  	{
          "from": 30,
          "to": 40,
          "color": "#ff8c1a"
      },
      {
          "from": 40,
          "to": 150,
          "color": "#33cc33"
      },
       {
          "from": 210,
          "to": 320,
          "color": "#e60000"
      },
       {
          "from":320,
          "to": 330,
          "color": "#ff8c1a"
      }
      
  ],
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#007F80",
  colorNeedleEnd: "#007F80",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 300,
  animationRule: "linear"
}).draw();
  

var labelAws = document.getElementById('label-aws');

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var awa = myObj.awa;
      var aws = myObj.aws;
      labelAws.textContent = aws.toFixed(1);
      gaugeAwa.value = awa;
      gaugeAwa.update({ valueText: myObj.awa, animationDuration: 300 });
      
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    var aws = myObj.aws;
    labelAws.textContent = aws.toFixed(1);
    gaugeAwa.value = myObj.awa;
    gaugeAwa.update({ valueText: myObj.awa, animationDuration: 300 })
    
  }, false);
}