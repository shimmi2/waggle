<div class="card">
  <h2>Průběh zpracování</h2>
  <p>Server pošle <b>jednu dávku na každé procento</b> a mezi nimi spí 0,1 s.</p>

  <div class="progress"><div id="pb" class="progress-bar" role="progressbar"
       style="width: 0%" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100"><span id="pb_pct">0 %</span></div></div>
  <div id="pb_note">&nbsp;</div>

  <p>
    <a href="#?function=progress_run"  data-fw id="run_btn">Spustit streamem</a> &nbsp;
    <a href="#?function=progress_push" data-fw id="run_push">Spustit pushem</a>
  </p>
  <p><small><b>Streamem</b> drží PHP spojení 10 s a Apache dávky slepuje po ~4 s.
     <b>Pushem</b> odpoví PHP za ~20 ms a procenta doručí nginx přes SSE.
     Push vyžaduje nchan; bez něj vrátí endpoint 503.</small></p>
</div>
