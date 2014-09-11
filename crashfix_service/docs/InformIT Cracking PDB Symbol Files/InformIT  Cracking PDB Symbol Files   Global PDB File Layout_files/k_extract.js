var ns_k_description = "";
var ns_k_keywords = "";
var ns_k_title = document.title;
var ns_k_url = document.location;
var metas=document.getElementsByTagName('head')[0].getElementsByTagName('meta'); 
for(var i=0,L=metas.length;i<L;i++){ 
	  if(metas[i].name.toLowerCase()=='description') ns_k_description=metas[i].content; 
	  if(metas[i].name.toLowerCase()=='keywords') ns_k_keywords=metas[i].content; 
} 
var ns_k_url = "http://adv.netshelter.net/context_keywords/k_log.php?s="+ns_k_siteid+"&u="+escape(escape(ns_k_url))+"&t="+escape(escape(ns_k_title))+"&d="+escape(escape(ns_k_description))+"&k="+escape(escape(ns_k_keywords));
document.write(unescape('%3Cscript type="text/javascript" src="'+ns_k_url+'"%3E%3C/script%3E'));
