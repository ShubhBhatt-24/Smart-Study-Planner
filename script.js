// Team Avengers — Browser interface. All planning answers come from C++.
let state = null;
let currentPage = 'overview';
const $ = id => document.getElementById(id);
const esc = value => String(value ?? '').replace(/[&<>"']/g, char => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[char]));
const plural = (count,singular,pluralWord=singular+'s') => count===1 ? singular : pluralWord;
let toastTimer;
function notify(message, isError=false) {
  const node=$('toast'); node.textContent=message;node.classList.toggle('error',isError);node.classList.add('show');
  clearTimeout(toastTimer);toastTimer=setTimeout(()=>node.classList.remove('show'),3300);
}
async function api(path,values) {
  const options=values ? {method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(values).toString()} : undefined;
  const response=await fetch(path,options);
  if(!response.ok) throw new Error('The local server did not respond.');
  const data=await response.json();
  if(!data.ok) throw new Error(data.error||'Something went wrong.');
  return data;
}
async function refresh() {
  try {state=await api('/api/state');render();}
  catch(error){notify(error.message,true);}
}
async function act(endpoint,fields,successMessage) {
  try {const result=await api(endpoint,fields);state=result;render();notify(successMessage);return true;}
  catch(error){notify(error.message,true);return false;}
}
function page(name) {
  currentPage=name;
  document.querySelectorAll('.page').forEach(node=>node.classList.toggle('active',node.id==='page-'+name));
  document.querySelectorAll('[data-page]').forEach(node=>node.classList.toggle('active',node.dataset.page===name));
  const names={overview:'Overview',tasks:'My tasks',schedule:'Smart scheduling',conflicts:'Deadline conflicts',recovery:'Adaptive recovery',risk:'Academic risks',whatif:'What-if simulator'};
  $('crumb').textContent=names[name];window.scrollTo({top:0,behavior:'smooth'});
}
function empty(text){return `<div class="empty-state">${esc(text)}</div>`;}
function taskRow(task,rank,metaRight) {
  return `<div class="list-row"><div class="rank">${rank}</div><div class="details"><b>${esc(task.name)}</b><small>${task.deadline===0?'Overdue':`Due in ${task.deadline} ${plural(task.deadline,'day')}`} · Importance ${task.importance}</small></div><span class="right-value">${metaRight||`${task.urgency} pts`}</span></div>`;
}
function planRows(blocks,dayFilter=0) {
  const visible=blocks.filter(b=>!dayFilter||b.day===dayFilter);
  if(!visible.length)return empty('No study blocks scheduled yet. Add a task to get started.');
  let day=0;return visible.map(b=>{
    let title=''; if(b.day!==day){day=b.day;title=`<div class="day-label">DAY ${day}</div>`;}
    return `${title}<div class="list-row"><div class="rank">${String(b.day).padStart(2,'0')}</div><div class="details"><b>${esc(b.name)}</b><small>Task #${b.id}</small></div><span class="right-value">${b.hours}h</span></div>`;
  }).join('');
}
function riskTag(text){return `<span class="tag ${text.toLowerCase()}">${esc(text)} RISK</span>`;}
function render(){
 if(!state)return;
 $('session-pill').textContent=`Session ${state.day}`;
 const remaining=state.tasks.reduce((n,t)=>n+t.remaining,0);
 const active=state.tasks.filter(t=>t.remaining>0).length;
 $('stat-total').textContent=String(active);
 $('stat-hours').innerHTML=`${remaining}<span>h</span>`;
 $('stat-conflicts').textContent=String(state.conflicts.length);
 $('stat-capacity').innerHTML=`${state.dailyHours}<span>h</span>`;
 $('overview-priority').innerHTML=state.priority.length?state.priority.slice(0,3).map((t,i)=>taskRow(t,i+1)).join(''):empty('Your priority list is empty. Add your first task.');
 $('overview-plan').innerHTML=planRows(state.plan.blocks,1);
 $('task-count').textContent=`${state.tasks.length} ${plural(state.tasks.length,'task')}`;
 $('task-table').innerHTML=state.tasks.length?state.tasks.map(t=>`<tr><td><b>${esc(t.name)}</b><br><small>#${t.id}${t.remaining===0?' · Completed':''}</small></td><td>${['','Low','Medium','High'][t.importance]}</td><td>${t.deadline===0?'Overdue':`${t.deadline}d`}</td><td>${t.remaining}h</td><td class="table-actions"><button class="action-button" data-progress="${t.id}" ${t.remaining===0?'disabled':''}>Log progress</button> <button class="action-button remove" data-remove="${t.id}">Delete</button></td></tr>`).join(''):`<tr><td colspan="5">No tasks added yet. Use the form to add your first task.</td></tr>`;
 $('priority-list').innerHTML=state.priority.length?state.priority.map((t,i)=>taskRow(t,i+1)).join(''):empty('Add tasks to calculate priority scores.');
 $('schedule-list').innerHTML=planRows(state.plan.blocks);
 $('schedule-note').textContent=`Unscheduled work: ${state.plan.unscheduled}h. Task progress does not change until you log actual study hours.`;
 const warning=state.conflicts.length>0;
 $('conflict-summary').innerHTML=`<div class="status-panel ${warning?'warning':''}"><b>${warning?`${state.conflicts.length} deadline ${plural(state.conflicts.length,'conflict')} detected`:'No deadline conflicts detected'}</b><p>${warning?'The cumulative work exceeds available hours at one or more deadlines. Adjust your workload or daily study capacity.':'Current pending tasks fit within your available study hours.'}</p></div>`;
 $('conflict-table').innerHTML=warning?state.conflicts.map(c=>`<tr><td><b>${c.deadline===0?'Overdue':`Day ${c.deadline}`}</b></td><td>${c.needed}h</td><td>${c.available}h</td><td><span class="tag high">${c.shortage}h short</span></td></tr>`).join(''):`<tr><td colspan="4">No conflicts for the current workload.</td></tr>`;
 $('missed-input').max=String(state.dailyHours);
 if(document.activeElement!==$('missed-input'))$('missed-input').value=String(state.missedToday);
 updateRecoveryTotals();
 $('recovery-result').innerHTML=`<div class="status-panel ${state.conflicts.length?'warning':''}"><b>${state.conflicts.length?'Study plan needs attention':'Recovery plan is feasible'}</b><p>${state.plan.unscheduled} hours unscheduled · ${state.conflicts.length} conflict warnings</p></div>${planRows(state.plan.blocks,1)}`;
 const counts={HIGH:0,MEDIUM:0,LOW:0};state.risks.forEach(r=>counts[r.risk]++);
 $('risk-high').textContent=counts.HIGH;$('risk-medium').textContent=counts.MEDIUM;$('risk-low').textContent=counts.LOW;
 $('risk-list').innerHTML=state.risks.length?state.risks.map(r=>{const t=state.tasks.find(t=>t.id===r.id);return `<div class="list-row"><div class="details"><b>${esc(t?.name||'Task')}</b><small>${t?.deadline===0?'Overdue':`Deadline in ${t?.deadline} days`} · ${t?.remaining}h remaining</small></div>${riskTag(r.risk)}</div>`;}).join(''):empty('No pending tasks to assess.');
 if(document.activeElement!==$('daily-hours-input'))$('daily-hours-input').value=String(state.dailyHours);
 $('real-setting').textContent=`${state.dailyHours} hours/day · ${state.missedToday} missed`;
}
function updateRecoveryTotals(){if(!state)return;const field=$('missed-input');const n=Number(field.value);$('recovery-planned').textContent=`${state.dailyHours}h`;$('recovery-available').textContent=`${Math.max(0,state.dailyHours-(Number.isFinite(n)?n:0))}h`;}
function formValues(form){return Object.fromEntries(new FormData(form).entries());}
function wireForm(id,path,success,after){$(id).addEventListener('submit',async event=>{event.preventDefault();const fields=formValues(event.currentTarget);const ok=await act(path,fields,success);if(ok&&after)after();});}
function showProgress(id){const t=state.tasks.find(t=>t.id===id);if(!t||!t.remaining)return;$('progress-id').value=String(id);$('progress-description').textContent=`${t.name} · ${t.remaining} hours remaining`;$('progress-hours').max=String(t.remaining);$('progress-hours').value=String(Math.min(t.remaining,1));$('progress-dialog').showModal();}
function renderScenario(data){const base=state.plan;const next=data.scenario;const originalTotal=state.conflicts.length;const simulatedTotal=next.conflicts.length;$('whatif-results').innerHTML=`<div class="compare-grid"><div class="compare-card"><span>CURRENT PLAN</span><strong>${originalTotal}</strong><small>${plural(originalTotal,'conflict')} · ${base.unscheduled}h unscheduled</small></div><div class="compare-card"><span>WHAT-IF PLAN</span><strong>${simulatedTotal}</strong><small>${plural(simulatedTotal,'conflict')} · ${next.unscheduled}h unscheduled</small></div></div><div class="status-panel ${simulatedTotal?'warning':''}" style="margin-top:14px"><b>${simulatedTotal?'Some deadlines still conflict':'All simulated deadlines are feasible'}</b><p>Scenario: ${data.hours}h per day, ${data.missed}h missed today. Saved settings are unchanged.</p></div><div class="stack">${planRows(next.blocks)}</div>`;}
function init(){
 document.querySelectorAll('[data-page]').forEach(el=>el.addEventListener('click',()=>page(el.dataset.page)));
 document.querySelectorAll('[data-go]').forEach(el=>el.addEventListener('click',()=>page(el.dataset.go)));
 wireForm('hours-form','/api/hours','Daily study hours updated.');
 $('next-day').addEventListener('click',()=>{if(confirm('Advance to the next study day? Deadlines will decrease by one day.'))act('/api/next',{},'Moved to the next study day.');});
 wireForm('task-form','/api/add','Task added successfully.',()=>$('task-form').reset());
 wireForm('recovery-form','/api/recovery','Study plan recalculated.');
 wireForm('progress-form','/api/progress','Study progress recorded.',()=>$('progress-dialog').close());
 $('cancel-progress').addEventListener('click',()=>$('progress-dialog').close());
 $('missed-input').addEventListener('input',updateRecoveryTotals);
 $('task-table').addEventListener('click',async event=>{const button=event.target.closest('button');if(!button)return;if(button.dataset.progress)showProgress(Number(button.dataset.progress));if(button.dataset.remove&&confirm('Delete this task?'))await act('/api/remove',{id:button.dataset.remove},'Task removed.');});
 $('demo-button').addEventListener('click',()=>act('/api/demo',{},'Three example tasks added.'));
 $('whatif-form').addEventListener('submit',async event=>{event.preventDefault();try{const result=await api('/api/whatif',formValues(event.currentTarget));renderScenario(result);notify('Simulation complete. Your saved plan is unchanged.');}catch(error){notify(error.message,true);}});
 window.addEventListener('hashchange',()=>{const hash=location.hash.slice(1);if(document.getElementById('page-'+hash))page(hash);});
 refresh();
}
document.addEventListener('DOMContentLoaded',init);
