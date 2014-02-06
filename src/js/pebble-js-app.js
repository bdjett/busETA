//
// DoubleMap for Pebble
// 

var maxAppMessageBuffer = 100;
var maxAppMessageTries = 3;
var appMessageRetryTimeout = 3000;
var appMessageTimeout = 100;
var httpTimeout = 12000;
var appMessageQueue = [];
var max_stops = 5;
var isNewList = false;
var latitude;
var longitude;

// The current buses on the map
// The array is indexed by the ID number of each bus, creating a sparse
// array.
var buses = [];

// busIds keeps a list of all active bus IDs in no particular order. It
// is a convenience for looping through all of the buses, but it must be
// kept syncronized with 'buses'.
var busIds = [];

// The currently-running routes
// The array is indexed by the ID number of each route,
var routes = [];
var routeIds = [];

// A list of stops
var stops = [];

// stopIds maps stopIds to stop objects
var stopIds = [];

// fetchRoutes
// -----------
//
// fetches all the routes data and calls fetchBuses when it has
// finished loading
var fetchRoutes = function() {
	console.log("Fetching all routes...");
	var userAgency = localStorage['doublemap_agency'].toString();
	var req = new XMLHttpRequest();
	if (userAgency) {
		var requestUrl = "http://" + userAgency + ".doublemap.com/map/v2/routes";
		console.log(requestUrl);
		req.open('GET', requestUrl, true);
		req.onload = function(e) {
			if (req.readyState == 4) {
				if (req.status == 200) {
					if (req.responseText) {
						var response = JSON.parse(req.responseText);
						response.forEach(function (element, index, array) {
							var r = element;
							// Add this fetched route to the routes array
							console.log(r.id.toString());
							routes[r.id] = r;
							// if we get a null stop list, just replace with empty list
							if (r.stops === null) {
								r.stops = [];
							}
							routeIds.push(r.id);
						});
					}
				}
			}
		};
		req.ontimeout = function() {
			console.log('HTTP request timed out');
			sendError('Request timed out!');
		};
		req.onerror = function() {
			console.log('HTTP request returned error');
			sendError('Failed to connect!');
		};
		req.send(null);
	}
}
										   
function getClosestStops() {
	if (navigator.geolocation) {
		console.log('finding location...');
		navigator.geolocation.getCurrentPosition(fetchStops,
                                             errorCallback,
                                             {maximumAge:600000, timeout:0});
	} else {
		console.log('no location support');
		sendError('Location not supported.');
	}
}	
										  

function errorCallback(error) {
	Pebble.showSimpleNotificationOnPebble("BusETA", "Location could not be found!");
}

// fetchStops
// ----------
//
// fetches all the stops data 
var fetchStops = function(position) {
	latitude = position.coords.latitude;
	longitude = position.coords.longitude;
	console.log(latitude.toString() + ", " + longitude.toString());
	console.log("Fetching all stops...");
	var userAgency = localStorage['doublemap_agency'].toString();
	var req = new XMLHttpRequest();
	if (userAgency) {
		var requestUrl = "http://" + userAgency + ".doublemap.com/map/v2/stops";
		req.open('GET', requestUrl, true);
		req.onload = function(e) {
			if (req.readyState == 4) {
				if (req.status == 200) {
					if (req.responseText) {
						var response = JSON.parse(req.responseText);
						count = 0;
						response.forEach(function (element, index, array) {
							var s = element;
							s.distance = haversine(latitude, longitude, s.lat, s.lon);
							stops[s.id] = s;
							stopIds.push(s.id);
						});
						stopIds.sort(function(a, b) { return stops[a].distance - stops[b].distance; });
						stopIds.forEach(function (element, index, array) {
							if (count < max_stops) {
								console.log(stops[element].name.toString());
								console.log(stops[element].distance.toString());
								appMessageQueue.push({'message': {'stopId':stops[element].id.toString(), 'stopName':stops[element].name.toString(), 'index':index}});
								count++;
							}
						});
					}
				}
			}
			sendAppMessage();
		};
		req.ontimeout = function() {
			console.log('HTTP request timed out');
			sendError('Request timed out!');
		};
		req.onerror = function() {
			console.log('HTTP request returned error');
			sendError('Failed to connect!');
		};
		req.send(null);
	}
}

// fetchStopRoutes
// ---------------
//
// fetches all the routes at a stop
var fetchStopRoutes = function(stopId) {
	console.log("Fetching routes for stopId " + stopId.toString());
	count = 0;
	routeIds.forEach(function (element, index, array) {
		var route = routes[element];
		route.stops.forEach(function (element, index, array) {
			var stop = stops[element];
			if (stop.id.toString() == stopId) {
				console.log('Pushing: ' + route.id.toString() + " " + route.name.toString() + " " + count.toString());
				appMessageQueue.push({'message': {'routeId':route.id.toString(), 'routeName':route.name.toString(), 'index':count}});
				count++;
				return false;
			}
		});
	});
	if (count == 0) {
		appMessageQueue.push({'message': {'error':'No routes for this stop.'}});
	}
	sendAppMessage();
}

// fetchETA
// --------
//
// fetches estimated time of arrival for the next bus
var fetchETA = function(stopId, routeId) {
	console.log("Fetching ETA for routeId " + routeId.toString() + " and stopId " + stopId.toString());
	var userAgency = localStorage['doublemap_agency'].toString();
	var req = new XMLHttpRequest();
	if (userAgency) {
		var requestUrl = "http://" + userAgency + ".doublemap.com/map/v2/eta?stop=" + stopId.toString();
		console.log(requestUrl);
		req.open('GET', requestUrl, true);
		req.onload = function(e) {
			if (req.readyState == 4) {
				if (req.status == 200) {
					if (req.responseText) {
						var response = JSON.parse(req.responseText);
						count = 0;
						response["etas"][stopId]["etas"].forEach(function (element, index, array) {
							if (element.route.toString() == routeId) {
								appMessageQueue.push({'message': {'eta':element.avg.toString(), 'index':count}});
								count++;
							}
						});
						if (count == 0) {
							appMessageQueue.push({'message': {'error':'No ETAs for this route.'}});
						}
					}
				}
			}
			sendAppMessage();
		};
		req.ontimeout = function() {
			console.log('HTTP request timed out');
			sendError('Request timed out!');
		};
		req.onerror = function() {
			console.log('HTTP request returned error');
			sendError('Failed to connect!');
		};
		req.send(null);
	}
}

// compareDist
// -----------
function compareDist(stop1Id, stop2Id) {
	console.log(stop1Id.toString() + " " + stop2Id.toString());
	stop1distance = haversine(latitude, longitude, stops[stop1Id].lat, stops[stop1Id].lon);
	stop2distance = haversine(latitude, longitude, stops[stop2Id].lat, stops[stop2Id].lon);
	return stop1distance - stop2distance;
}

// haversine
// ---------
//
// calculates the distance between two geograpic points and
// returns the result in meters
function haversine(lat1, lon1, lat2, lon2) {
	var R = 6371000; // radius of Earth in m
	var dLat = toRad(lat2-lat1);
	var dLon = toRad(lon2-lon1);
	var a = Math.sin(dLat/2) * Math.sin(dLat/2) + 
		Math.cos(toRad(lat1)) * Math.cos(toRad(lat2)) *
		Math.sin(dLon/2) * Math.sin(dLon/2);
	var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
	var d = R * c;
	return d;
}

// toRad
// -----
//
// converts degrees to radians
function toRad(degree) {
	return degree * Math.PI / 180;
}

// Pebble Event Listeners

// Pebble JS received ready notification
Pebble.addEventListener('ready', function(e) {
	routes = [];
	routeIds = [];
	stops = [];
	stopIds = [];
	if (localStorage['doublemap_agency']) {
		// notifyPebbleConnected(localStorage['doublemap_agency'].toString());
		console.log("Found selected agency!");
		fetchRoutes();
		getClosestStops();
	} else {
		console.log("no agency");
		Pebble.showSimpleNotificationOnPebble("BusETA", "Please select an agency in the Pebble app on your phone.");
	}
});

Pebble.addEventListener('appmessage', function(e) {
	if (e.payload.getRoutes) {
		fetchStopRoutes(e.payload.stopId.toString());
	} else if (e.payload.refresh) {
		getClosestStops();
	} else if (e.payload.getEta) {
		fetchETA(e.payload.stopId, e.payload.routeId);
	}
});
							
function sendAppMessage() {
	if (appMessageQueue.length > 0) {
		currentAppMessage = appMessageQueue[0];
		currentAppMessage.numTries = currentAppMessage.numTries || 0;
		currentAppMessage.transactionId = currentAppMessage.transactionId || -1;
		
		if (currentAppMessage.numTries < maxAppMessageTries) {
			console.log('Sending message');
			Pebble.sendAppMessage(
				currentAppMessage.message,
				 function(e) {
					 appMessageQueue.shift();
					 setTimeout(function() {
						 sendAppMessage();
					 }, appMessageTimeout);
				 }, function(e) {
					 console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
					 appMessageQueue[0].transactionId = e.data.transactionId;
					 appMessageQueue[0].numTries++;
					 setTimeout(function() {
						 sendAppMessage();
					 }, appMessageRetryTimeout);
				 }
			);
		} else {
			console.log('Failed sending AppMessage after multiple attempts for transactionId: ' + currentAppMessage.transactionId + '. Error: None. Here\'s the message: ' + JSON.stringify(currentAppMessage.message));
		}
	}
}
							
function notifyPebbleConnected(agency) {
	var transactionId = Pebble.sendAppMessage( { 'agency' : agency },
											   function(e) {
												 console.log('Successfully delieverd agency message with transactionId=' + e.data.transactionId);
											   },
											   function(e) {
												 console.log('Unable to deliver agency message with transactionId=' + e.data.transactionId + ' Error is: ' + e.error.message);
											   });
}

function sendError(e) {
	console.log("sending error");
}
							
							function cleanStops() {
								Pebble.sendAppMessage({'cleanStops': 1});
							}
							
Pebble.addEventListener("showConfiguration", function(e) {
	console.log("Showing config");
	Pebble.openURL("http://logicalpixels.com/doublemap/index.html#" + localStorage['doublemap_agency']);
});

Pebble.addEventListener("webviewclosed", function(e) {
	var configuration = JSON.parse(decodeURIComponent(e.response));
	console.log("Agency: " + configuration.agency.toString());
	localStorage['doublemap_agency'] = configuration.agency.toString();
	Pebble.sendAppMessage({'agency':localStorage['doublemap_agency'].toString()});
	routes = [];
	routeIds = [];
	stops = [];
	stopIds = [];
	fetchRoutes();
	getClosestStops();
});