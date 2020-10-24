#include <Arduino.h>
const char style_css[] PROGMEM = R"=====(
* {
    font-family: 'Source Code Pro', UltimaPro, Verdana, Arial, sans-serif;
    color: DarkSlateGrey;
    vertical-align: baseline;
    box-sizing: border-box;
}
.clearfix {
    overflow: auto;
}
.clearfix::before,
.clearfix::after {
    content: "";
    display: table;
} 
.clearfix::after {
    clear: both;
}
body {
    margin: 0px;
    padding: 0px;
    max-width: 100%;
    color: #4a4a4a;
    font-weight: 400;
    line-height: 1.5;
    font-size: 1.0em;
}
html {
    margin: 0px;
    padding: 0px;
    max-width: 100%;
    color: #4a4a4a;
    font-weight: 400;
    line-height: 1.5;
    font-size: 1.0em;
}
.navBar {
    margin: 0;
    padding: 0;
    background-color: white;
    z-index: 1;
    box-shadow: 0 .5em .5em -.125em rgba(10, 10, 10, .1), 0 0 0 1px rgba(10, 10, 10, .02);
    overflow: hidden;
    position: fixed;
    width: 100%;
    top: 0px;
    line-height: 35px;
    vertical-align: bottom;
    font-size: 1.2em;
}
.navBar a {
    float: left;
    display: block;
    color: #33C3F0;
    text-align: center;
    padding: 0px 10px;
    text-decoration: none;
    -webkit-tap-highlight-color: rgba(200, 0, 0, 0.4);
    border-radius: 5px;
    margin: 18px 10px;
}
.navBar a:hover:not(.active) {
    background-color: #245BA9;
    color: white;
}
.navBar a.active {
    background-color: #33C3F0;
    color: white;
}
.navBar .drawer {
    display: none;
}
.navBar li.right {
    float: right;
}
.navBar .logo {
    float: left;
    z-index: 10000;
    margin: 14px;
    background-color: transparent;
}
.navBar .title {
    float: right;
    color: #33C3F0;
    z-index: 10000;
    padding-right: 20px;
    background-color: transparent;
    margin: 18px 10px;
}
.navBar .icon {
    display: none;
}
header:after {
    content: "";
    display: table;
    clear: both;
    margin-top: 20px;
}
section {
    margin: 0px auto;
    text-align: center;
}
section:first-of-type {
    margin: 100px 0 10px 0;
}
section:after {
    content: "";
    display: table;
    clear: both;
    margin-top: 30px;
}
.tabcontent {
    display: none;
    animation: fadeEffect 1.5s; 
}
@keyframes fadeEffect {
    from {opacity: 0;}
    to {opacity: 1;}
}
.row {
    text-align: center;
    margin: 0px auto;
    display: inline-block;
}
.row:after {
    content: "";
    display: table;
    clear: both;
    margin-top: 20px;
}
form {
    max-width: 500px;
    padding: 20px;
    margin: auto;
    background-color: rgba(207,203,207,0.2);
    border-radius: 10px;
    box-sizing: border-box;
}
form .row {
    text-align: left;
    display: table;
    width: 100%;
}
.col-25 {
    width: 25%;
}
.col-33 {
    width: 33.33333%;
}
.col-50 {
    width: 50%;
}
.col-75 {
    width: 75%;
}
.col-100 {
    width: 99%;
}
[class*='col-'] {
    float: left;
    padding: 2px 6px;
    margin-top: 6px;
}
[class*='col-']:last-of-type {
}
label {
    float: right; 
    padding: 0px 6px;
    display: inline-block;
    white-space: nowrap;
}
.log {
    background-color: black; 
    color: white; 
    font-family: monospace; 
    text-align: left; 
    padding: 6px;
}
table {
    margin: 0px auto;
    width: 100%;
    font-size: 0.95em;
}
th,
td {
    text-align: left;
    padding: 4px;
    height: 1px;
}
th:first-child,
td:first-child {
    text-align: right;
    padding: 2px 4px 2px 8px;
}
tbody tr:nth-child(2n) {
    background-color: rgba(240, 240, 240, 0.42);
}
tr:hover {
    background-color: rgba(230, 230, 230, 0.5);
}
td:hover {
    __border-right: 2px solid #2db6eb;
}
#info table {
    width: 400px;
}
#info td:first-child {
    width: 50%;
}
#info {
    text-align: left;
    padding: 6px;
}
#messages table {
    width: 800px;
}
#messages th:first-child,
#messages td:first-child,
#info th:first-child,
#info td:first-child {
    text-align: right;
    padding-right: 16px;
}
#messages {
    text-align: left;
    padding: 6px;
}
.button,
button,
input[type="submit"],
input[type="reset"],
input[type="button"] {
    font-size: 1em;
    display: inline-block;
    padding: 6px 12px;
    text-align: center;
    text-decoration: none;
    white-space: nowrap;
    background-color: #2db6eb;
    color: white;
    border: 0;
    border-radius: 5px;
    box-sizing: border-box;
    cursor: pointer;
    margin: 0px 10px;
}
.button:hover,
button:hover,
input[type="submit"]:hover,
input[type="reset"]:hover,
input[type="button"]:hover {
    background-color: #245BA9;
}
input[type="text"] {
    font-size: 0.95em;
    width: 100%;
    color: gray;
}
.left {
    float: left;
}
.right {
    float: right;
}
.center {
    display: table;
    margin: 0 auto;
    vertical-align: middle;
    text-align: center;
}
.debug {
    font-family: monaco;
    font-size: 0.8em;
    line-height: 13px;
    color: #e0e0e0;
}   
.error {
    color: red;
}
/* Tooltip container */
.tooltip {
    position: relative;
    display: inline-block;
}
.tooltip .tooltiptext {
    visibility: hidden;
    /* width: 60%; */
    background-color: #555;
    color: #fff;
    text-align: center;
    font-family: monaco;
    font-size: 0.8em;
    padding: 5px;
    border-radius: 6px;

    /* Position the tooltip text */
    position: fixed;
    z-index: 1;
    top: 120px;
    right: 25%;
    /* margin-left: -60px; */

    /* Fade in tooltip */
    opacity: 0;
    transition: opacity 0.3s;
}
.tooltip .tooltiptext::after {
    content: "";
    position: absolute;
    top: 100%;
    left: 50%;
    margin-left: -5px;
    border-width: 5px;
    border-style: solid;
    border-color: #555 transparent transparent transparent;
}
.tooltip:hover .tooltiptext {
    visibility: visible;
    opacity: 1;
}
@media only screen and (max-width: 768px) {
    .row {
        width: 90%;
        white-space: normal;
    }
    .title {
        display: none;
    }
    #info table {
        white-space: normal;
        width: 90%;
    }
    #info td {
        white-space: normal;
    }
    #messages table {
        white-space: normal;
        width: 90%;
    }
    #messages th:nth-child(2),
    #messages td:nth-child(2),
    #messages th:nth-child(5),
    #messages td:nth-child(5),
    #messages th:nth-child(6),
    #messages td:nth-child(6) {
        display: none;
    }
    #messages td {
        white-space: normal;
    }
    #debug {
        font-size: 0.7em;
    }
}
@media only screen and (max-width: 400px) {
    .row {
        width: 90%;
        white-space: normal;
    }
    .navBar a {display: none;}
    .navBar a.drawer {
        float: right;
        display: block;
    }
    .navBar.responsive {position: relative;}
    .navBar.responsive a.drawer {
        position: absolute;
        right: 0;
        top: 0;
    }
    .navBar.responsive .logo {
        display: none;
    }
    .navBar.responsive a {
        float: none;
        display: block;
        text-align: left;
    }
    .title {
        display: none;
    }
    #info table {
        white-space: normal;
        width: 90%;
    }
    #info td {
        white-space: normal;
    }
    #messages table {
        white-space: normal;
        width: 90%;
    }
    #messages th:nth-child(-n+2),
    #messages td:nth-child(-n+2),
    #messages th:nth-child(5),
    #messages td:nth-child(5),
    #messages th:nth-child(6),
    #messages td:nth-child(6) {
        display: none;
    }
    #messages td {
        white-space: normal;
        max-width: 30px;
    }
    #debug {
        font-size: 0.7em;
    }
}
.loader, .loader:after {
    border-radius: 50%;
    width: 10em;
    height: 10em;
}
.loader {
    position: fixed;
    z-index: 999;
    overflow: show;
    margin: auto;
    top: 0;
    left: 0;
    bottom: 50%;
    right: 0;
    font-size: 10px;
    text-indent: -9999em;
    border-top: 1.1em solid rgba(160, 160, 160, 0.2);
    border-right: 1.1em solid rgba(160, 160, 160, 0.2);
    border-bottom: 1.1em solid rgba(160, 160, 160, 0.2);
    border-left: 1.1em solid #ffffff;
    -webkit-transform: translateZ(0);
    transform: translateZ(0);
    -webkit-animation: load8 5.1s infinite linear;
    animation: load8 5.1s infinite linear;    
}
@-webkit-keyframes load8 {
    0% {
        -webkit-transform: rotate(0deg);
        transform: rotate(0deg);
    }
    100% {
        -webkit-transform: rotate(360deg);
        transform: rotate(360deg);
    }
}
@keyframes load8 {
    0% {
        -webkit-transform: rotate(0deg);
        transform: rotate(0deg);
    }
    100% {
        -webkit-transform: rotate(360deg);
        transform: rotate(360deg);
    }
}
)=====";
