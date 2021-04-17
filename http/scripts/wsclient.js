var ws;
function WebSocketClient()
{
	try {
		ws=new WebSocket("ws://" + location.hostname + ":" + location.port, "simple-json");
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
		var obj;
		try {
			obj=JSON.parse(evt.data);
		}
		catch(err) {
			document.getElementById("logWindow").innerHTML=
			err.message + document.getElementById("logWindow").innerHTML;
		}
		if(!obj.Transmission.Suspended) {
			document.getElementById("SuspendBtn").disabled=false;
			document.getElementById("ResumeBtn").disabled=true;
		} else {
			document.getElementById("SuspendBtn").disabled=true;
			document.getElementById("ResumeBtn").disabled=false;
		}
		if(obj.Transmission.Starting) {
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
		var str;
		try {
			str=JSON.stringify(obj, null, 2);
		}
		catch(err) {
			str=err.message;
		}
		finally {
			tag="<pre>" + str + "</pre>";
			document.getElementById("logWindow").innerHTML=tag;
		}
	}
}

function BtnHandler()
{
	var thatId=document.activeElement.id;
	ws.send(JSON.stringify(thatId));
}
