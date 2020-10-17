#include <Arduino.h>

const char reboot_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
	<title>MySensors Wifi Gateway</title>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
	<link rel="shortcut icon" type="image/x-icon" href="favicon.ico">
	<link rel="mask-icon" type="image/svg" href="mask-icon.svg" color="#48A1BE">
	<meta name="theme-color" content="#ffffff">	
	<link rel="stylesheet" type="text/css" href="style.css"/>
</head>
<body onload=onBodyLoad()>
<header>
	<nav>
			<div class="navBar" id="mainNavBar">
			<img class="logo" src='data:image/svg+xml;utf8,<svg style="fill-rule:evenodd;clip-rule:evenodd;stroke-linecap:round;stroke-linejoin:round;" xmlns:vectornator="http://vectornator.io" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns="http://www.w3.org/2000/svg" xml:space="preserve" version="1.1" viewBox="0 0 256 256"><path stroke="#00a3c2" stroke-width="20.99" d="M128+242.89C191.452+242.89+242.89+191.452+242.89+128C242.89+64.5481+191.452+13.11+128+13.11C64.5481+13.11+13.11+64.5481+13.11+128C13.11+191.452+64.5481+242.89+128+242.89Z" fill="none" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path stroke="#f59e13" fill-rule="evenodd" stroke-width="8" d="M31.3777+141.266L31.3777+107.818C31.3777+105.943+32.0437+104.29+33.3757+102.86C34.7077+101.429+36.311+100.714+38.1857+100.714C40.0603+100.714+41.7377+101.429+43.2177+102.86C44.6977+104.29+45.4377+105.943+45.4377+107.818L45.4377+143.042C45.4377+149.652+47.411+155.03+51.3577+159.174C55.3043+163.318+60.139+165.39+65.8617+165.39C71.683+165.39+76.567+163.367+80.5137+159.322C84.4603+155.276+86.4337+149.85+86.4337+143.042L86.4337+107.818C86.4337+105.943+87.1243+104.29+88.5057+102.86C89.887+101.429+91.515+100.714+93.3897+100.714C95.2643+100.714+96.917+101.429+98.3477+102.86C99.7783+104.29+100.493+105.943+100.493+107.818L100.493+143.042C100.493+149.85+102.442+155.276+106.339+159.322C110.237+163.367+115.096+165.39+120.917+165.39C126.739+165.39+131.623+163.367+135.569+159.322C139.516+155.276+141.489+149.85+141.489+143.042L141.489+107.818C141.489+105.943+142.205+104.29+143.635+102.86C145.066+101.429+146.719+100.714+148.593+100.714C150.468+100.714+152.121+101.429+153.551+102.86C154.982+104.29+155.697+105.943+155.697+107.818L155.697+141.266C155.697+152.02+152.367+160.777+145.707+167.536C139.047+174.294+130.735+177.674+120.769+177.674C110.212+177.674+101.085+172.938+93.3897+163.466C85.891+172.938+76.715+177.674+65.8617+177.674C55.995+177.674+47.781+174.294+41.2197+167.536C34.6583+160.777+31.3777+152.02+31.3777+141.266Z" fill="#f59e13" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path stroke="#f59e13" fill-rule="evenodd" stroke-width="8.98" d="M147.927+74.0739L173.308+74.0739C189.927+74.0739+203.018+78.9332+212.58+88.6519C222.141+98.3706+226.922+110.63+226.922+125.43C226.922+140.23+222.141+152.637+212.58+162.652C203.018+172.667+189.927+177.674+173.308+177.674L147.927+177.674C146.025+177.674+144.448+177.033+143.197+175.75C141.945+174.467+141.319+172.839+141.319+170.866L141.319+80.8819C141.319+78.9086+141.945+77.2806+143.197+75.9979C144.448+74.7152+146.025+74.0739+147.927+74.0739ZM173.308+86.6539L155.436+86.6539L155.436+165.094L173.308+165.094C186.223+165.094+196.11+161.271+202.968+153.624C209.826+145.977+213.256+136.579+213.256+125.43C213.256+114.281+209.851+105.031+203.043+97.6799C196.235+90.3292+186.323+86.6539+173.308+86.6539Z" fill="#f59e13" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path d="M130.204+81.3807C129.57+71.7308+136.878+63.3934+146.528+62.7585C156.178+62.1238+164.516+69.4318+165.149+79.0817C165.785+88.7317+158.477+97.0691+148.827+97.7039C139.177+98.3386+130.839+91.0306+130.204+81.3807Z" fill-rule="evenodd" fill="#00a3c2" opacity="1"/></svg>
' width="42" height="42">
			<a class="tablinks" id="defaultOpen" onclick="doTab(event, 'tab_home')" href="#"> home </a>
			<a class="tablinks" onclick="doTab(event, 'tab_stats')" href="#"> stats </a>
			<a class="tablinks" onclick="doTab(event, 'tab_network')" href="#"> network </a>
			<a class="tablinks" onclick="doTab(event, 'tab_settings')" href="#"> settings </a>
			<span class="title">MySensors Wifi Gateway</span>
			<a href="javascript:void(0);" class="drawer" onclick="openDrawerMenu()">&#9776;</a>
		</div>
	</nav>
</header>

<section>    
	<div class="row">
        <form method='POST' action='/reboot' enctype='multipart/form-data'>
    	<div class="row">
    		<div class="col-50"> 
                <label for="submit">ok to reboot?</label>
    		</div>
            <div class="col-50"> 
		        <input id="submit" type='submit' value='reboot'>
    		</div>
    	</div>
        </form>
	</div>
</section>
        
</div>
</body>
<script type="text/javascript"> 
	function onBodyLoad(){
	}
	function doTab(evt, section) {
		var i, tabcontent, tablinks;
		tabcontent = document.getElementsByClassName("tabcontent");
		for (i = 0; i < tabcontent.length; i++) {
			tabcontent[i].style.display = "none";
		}
		tablinks = document.getElementsByClassName("tablinks");
			for (i = 0; i < tablinks.length; i++) {
		tablinks[i].className = tablinks[i].className.replace(" active", "");
		}
		document.getElementById(section).style.display = "block";
		evt.currentTarget.className += " active";
		ws.send(section);
	}
	function openDrawerMenu(){
		var x = document.getElementById("mainNavBar");
		if (x.className === "navBar"){
			x.className += " responsive";
		} else {
			x.className = "navBar";
		}
	}
</script>
</html>
)=====";

