/**
* var disqus_shortname; (define in xsl)
*/
(function () {
  var s = document.createElement('script'); s.async = true;
  s.src = 'http://disqus.com/forums/' + disqus_shortname + '/count.js';
  (document.getElementsByTagName('HEAD')[0] || document.getElementsByTagName('BODY')[0]).appendChild(s);
}());