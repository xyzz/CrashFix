//<!-- START OF SmartSource Data Collector TAG -->
//<!-- Copyright (c) 1996-2008 WebTrends Inc.  All rights reserved. -->
//<!-- InformIT V8.0 -->
//<!-- $DateTime: 2007/03/05 13:33:14 $ -->
//<!-- ----------------------------------------------------------------------------------- -->
//<!-- Warning: The two script blocks below must remain inline. Moving them to an external -->
//<!-- JavaScript include file can cause serious problems with cross-domain tracking.      -->
//<!-- ----------------------------------------------------------------------------------- -->
var gDomain="statse.webtrendslive.com";
var gDcsId="dcs1slwc200000cl8qmbbfxqx_5x2u";

if ((typeof(gConvert)!="undefined")&&gConvert&&(document.cookie.indexOf(gFpc+"=")==-1)&&(document.cookie.indexOf("WTLOPTOUT=")==-1)){
	document.write("<SCR"+"IPT TYPE='text/javascript' SRC='"+"http"+(window.location.protocol.indexOf('https:')==0?'s':'')+"://"+gDomain+"/"+gDcsId+"/wtid.js"+"'><\/SCR"+"IPT>");
}

var gImages=new Array;
var gIndex=0;
var DCS=new Object();
var WT=new Object();
var DCSext=new Object();
var gQP=new Array();
var gI18n=false;
if (window.RegExp){
	var RE=gI18n?{"%25":/\%/g}:{"%09":/\t/g,"%20":/ /g,"%23":/\#/g,"%26":/\&/g,"%2B":/\+/g,"%3F":/\?/g,"%5C":/\\/g,"%22":/\"/g,"%7F":/\x7F/g,"%A0":/\xA0/g};
	if (gI18n){
		var EXRE=/dcs(uri)|(ref)|(aut)|(met)|(sta)|(sip)|(pro)|(byt)|(dat)|(p3p)|(cfg)|(redirect)|(cip)/i;
	}
}

// Add customizations here
function dcsVar(){
	var dCurrent=new Date();
	WT.tz=(dCurrent.getTimezoneOffset()/60*-1)||"0";
	WT.bh=dCurrent.getHours()||"0";
	WT.ul=navigator.appName=="Netscape"?navigator.language:navigator.userLanguage;
	if (typeof(screen)=="object"){
		WT.cd=navigator.appName=="Netscape"?screen.pixelDepth:screen.colorDepth;
		WT.sr=screen.width+"x"+screen.height;
	}
	if (typeof(navigator.javaEnabled())=="boolean"){
		WT.jo=navigator.javaEnabled()?"Yes":"No";
	}
	if (document.title){
		if (window.RegExp){
			var tire=new RegExp("^"+window.location.protocol+"//"+window.location.hostname+"\\s-\\s");
			WT.ti=document.title.replace(tire,"");
		}
		else{
			WT.ti=document.title;
		}
	}
	WT.js="Yes";
	WT.jv=dcsJV();
	if (document.body&&document.body.addBehavior){
		document.body.addBehavior("#default#clientCaps");
		WT.ct=document.body.connectionType||"unknown";
		document.body.addBehavior("#default#homePage");
		WT.hp=document.body.isHomePage(location.href)?"1":"0";
	}
	else{
		WT.ct="unknown";
	}
	if (document.all){
		WT.bs=document.body?document.body.offsetWidth+"x"+document.body.offsetHeight:"unknown";
	}
	else{
		WT.bs=window.innerWidth+"x"+window.innerHeight;
	}
	WT.fi="No";
	if (window.ActiveXObject){
		for(var i=10;i>0;i--){
			try{
				var flash = new ActiveXObject("ShockwaveFlash.ShockwaveFlash."+i);
				WT.fi="Yes";
				WT.fv=i+".0";
				break;
			}
			catch(e){
			}
		}
	}
	else if (navigator.plugins&&navigator.plugins.length){
		for (var i=0;i<navigator.plugins.length;i++){
			if (navigator.plugins[i].name.indexOf('Shockwave Flash')!=-1){
				WT.fi="Yes";
				WT.fv=navigator.plugins[i].description.split(" ")[2];
				break;
			}
		}
	}
	if (gI18n){
		if (typeof(document.defaultCharset)=="string"){
			WT.le=document.defaultCharset;
		} 
		else if (typeof(document.characterSet)=="string"){
			WT.le=document.characterSet;
		}
		else{
			WT.le="unknown";
		}
	}
	WT.tv="8.0.3";
	WT.sp="@@SPLITVALUE@@";
	DCS.dcsdat=dCurrent.getTime();
	DCS.dcssip=window.location.hostname;
	DCS.dcsuri=window.location.pathname;
	if (window.location.search){
		DCS.dcsqry=window.location.search;
		if (gQP.length>0){
			for (var i=0;i<gQP.length;i++){
				var pos=DCS.dcsqry.indexOf(gQP[i]);
				if (pos!=-1){
					var front=DCS.dcsqry.substring(0,pos);
					var end=DCS.dcsqry.substring(pos+gQP[i].length,DCS.dcsqry.length);
					DCS.dcsqry=front+end;
				}
			}
		}
	}
	if ((window.document.referrer!="")&&(window.document.referrer!="-")){
		if (!(navigator.appName=="Microsoft Internet Explorer"&&parseInt(navigator.appVersion)<4)){
			DCS.dcsref=window.document.referrer;
		}
	}
}

function dcsA(N,V){
	if (gI18n&&!EXRE.test(N)){
		if (N=="dcsqry"){
			var newV="";
			var params=V.substring(1).split("&");
			for (var i=0;i<params.length;i++){
				var pair=params[i];
				var pos=pair.indexOf("=");
				if (pos!=-1){
					var key=pair.substring(0,pos);
					var val=pair.substring(pos+1);
					if (i!=0){
						newV+="&";
					}
					newV+=key+"="+dcsEncode(val);
				}
			}
			V=V.substring(0,1)+newV;
		}
		else{
			V=dcsEncode(V);
		}
	}
	return "&"+N+"="+dcsEscape(V, RE);
}

function dcsEscape(S, REL){
	if (typeof(REL)!="undefined"){
		S=S.toString();
		for (var R in REL){
			S=S.replace(REL[R],R);
		}
		return S;
	}
	else{
		return escape(S);
	}
}

function dcsEncode(S){
	return (typeof(encodeURIComponent)=="function")?encodeURIComponent(S):escape(S);
}

function dcsCreateImage(dcsSrc){
	if (document.images){
		gImages[gIndex]=new Image;
		gImages[gIndex].src=dcsSrc;
		gIndex++;
	}
	else{
		document.write('<IMG ALT="" BORDER="0" NAME="DCSIMG" WIDTH="1" HEIGHT="1" SRC="'+dcsSrc+'">');
	}
}

function dcsMeta(){
	var elems;
	if (document.all){
		elems=document.all.tags("meta");
	}
	else if (document.documentElement){
		elems=document.getElementsByTagName("meta");
	}
	if (typeof(elems)!="undefined"){
		var length=elems.length;
		for (var i=0;i<length;i++){
			var name=elems.item(i).name;
			var content=elems.item(i).content;
			var equiv=elems.item(i).httpEquiv;
			if (name.length>0){
				if (name.indexOf("WT.")==0){
					WT[name.substring(3)]=content;
				}
				else if (name.indexOf("DCSext.")==0){
					DCSext[name.substring(7)]=content;
				}
				else if (name.indexOf("DCS.")==0){
					DCS[name.substring(4)]=content;
				}
			}
		}
	}
}

function dcsTag(){
	if (document.cookie.indexOf("WTLOPTOUT=")!=-1){
		return;
	}
	var P="http"+(window.location.protocol.indexOf('https:')==0?'s':'')+"://"+gDomain+(gDcsId==""?'':'/'+gDcsId)+"/dcs.gif?";
	if (gI18n){
		WT.dep="";
	}
	for (var N in DCS){
		if (DCS[N]){
			P+=dcsA(N,DCS[N]);
		}
	}
	var keys=["co_f","vt_sid","vt_f_tlv"];
	for (var i=0;i<keys.length;i++){
		var key=keys[i];
		if (WT[key]){
			P+=dcsA("WT."+key,WT[key]);
			delete WT[key];
		}
	}
	for (N in WT){
		if (WT[N]){
			P+=dcsA("WT."+N,WT[N]);
		}
	}
	for (N in DCSext){
		if (DCSext[N]){
			if (gI18n){
				WT.dep=(WT.dep.length==0)?N:(WT.dep+";"+N);
			}
			P+=dcsA(N,DCSext[N]);
		}
	}
	if (gI18n&&(WT.dep.length>0)){
		P+=dcsA("WT.dep",WT.dep);
	}
	if (P.length>2048&&navigator.userAgent.indexOf('MSIE')>=0){
		P=P.substring(0,2040)+"&WT.tu=1";
	}
	dcsCreateImage(P);
}

function dcsJV(){
	var agt=navigator.userAgent.toLowerCase();
	var major=parseInt(navigator.appVersion);
	var mac=(agt.indexOf("mac")!=-1);
	var ff=(agt.indexOf("firefox")!=-1);
	var ff0=(agt.indexOf("firefox/0.")!=-1);
	var ff10=(agt.indexOf("firefox/1.0")!=-1);
	var ff15=(agt.indexOf("firefox/1.5")!=-1);
	var ff2up=(ff&&!ff0&&!ff10&!ff15);
	var nn=(!ff&&(agt.indexOf("mozilla")!=-1)&&(agt.indexOf("compatible")==-1));
	var nn4=(nn&&(major==4));
	var nn6up=(nn&&(major>=5));
	var ie=((agt.indexOf("msie")!=-1)&&(agt.indexOf("opera")==-1));
	var ie4=(ie&&(major==4)&&(agt.indexOf("msie 4")!=-1));
	var ie5up=(ie&&!ie4);
	var op=(agt.indexOf("opera")!=-1);
	var op5=(agt.indexOf("opera 5")!=-1||agt.indexOf("opera/5")!=-1);
	var op6=(agt.indexOf("opera 6")!=-1||agt.indexOf("opera/6")!=-1);
	var op7up=(op&&!op5&&!op6);
	var jv="1.1";
	if (ff2up){
		jv="1.7";
	}
	else if (ff15){
		jv="1.6";
	}
	else if (ff0||ff10||nn6up||op7up){
		jv="1.5";
	}
	else if ((mac&&ie5up)||op6){
		jv="1.4";
	}
	else if (ie5up||nn4||op5){
		jv="1.3";
	}
	else if (ie4){
		jv="1.2";
	}
	return jv;
}

function dcsFunc(func){
	if (typeof(window[func])=="function"){
		window[func]();
	}
}

dcsVar();
dcsMeta();
dcsFunc("dcsAdv");
dcsTag();
//<!-- END OF SmartSource Data Collector  TAG -->	
