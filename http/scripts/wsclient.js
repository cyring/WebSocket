var ws;
function WebSocketClient()
{
	try {
		ws=new WebSocket("ws://" + location.hostname + ":8080", "simple-json");
	}
	catch(err) {
		document.getElementById("socketState").innerHTML="WebSockets [" + err + "]";
	}
	ws.onopen=function(evt)
	{
		document.getElementById("socketState").innerHTML="WebSockets [" + evt.type + "]";
		document.getElementById("SuspendBtn").disabled=false;
		document.getElementById("WSCloseBtn").disabled=false;
		document.getElementById("WSOpenBtn").disabled=true;
	}

	ws.onclose=function(evt)
	{
		document.getElementById("socketState").innerHTML="WebSockets [" + evt.type + "]";
		document.getElementById("SuspendBtn").disabled=true;
		document.getElementById("ResumeBtn").disabled=true;
		document.getElementById("WSCloseBtn").disabled=true;
		document.getElementById("WSOpenBtn").disabled=false;
	}
	ws.onmessage=function(evt)
	{
		var obj=JSON.parse(evt.data);
		if(!obj.Transmission.Suspended) {
			document.getElementById("SuspendBtn").disabled=false;
			document.getElementById("ResumeBtn").disabled=true;
		} else {
			document.getElementById("SuspendBtn").disabled=true;
			document.getElementById("ResumeBtn").disabled=false;
		}
		if(obj.Transmission.Starting) {
			document.getElementById("Core").innerHTML=obj.ProcInfo.Core;
			document.getElementById("Model").innerHTML=obj.ProcInfo.Model;
			document.getElementById("Kernel").innerHTML=obj.SysInfo.Kernel;
		} else {
			if(!obj.Transmission.Suspended) {
				document.getElementById("Processes").innerHTML=obj.SysInfo.Processes;
				document.getElementById("TotalRAM").innerHTML=obj.SysInfo.Memory.Total;
				document.getElementById("FreeRAM").innerHTML=obj.SysInfo.Memory.Free;
				document.getElementById("SharedRAM").innerHTML=obj.SysInfo.Memory.Shared;
				document.getElementById("BufferRAM").innerHTML=obj.SysInfo.Memory.Buffer;
			}
		}
		tag="<pre>" + JSON.stringify(obj, null, 2) + "</pre>";
		document.getElementById("logWindow").innerHTML=tag;
	}
}

function BtnHandler()
{
	var thatId=document.activeElement.id;
	ws.send(JSON.stringify(thatId));
}
