"""Integration tests: compile and exercise the real C++ HTTP server."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import urllib.parse
import urllib.request
import urllib.error
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASE = 'http://127.0.0.1:18081'

class PlannerIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix='avengers_test_')
        cls.work=Path(cls.tmp.name)
        cls.exe=cls.work/'planner_server'
        if os.name == 'nt':
            cls.exe=cls.work/'planner_server.exe'
            cmd=['g++','-std=c++17','-Wall','-Wextra',str(ROOT/'backend/server.cpp'),'-o',str(cls.exe),'-lws2_32']
        else:
            cmd=['g++','-std=c++17','-Wall','-Wextra','-pedantic',str(ROOT/'backend/server.cpp'),'-o',str(cls.exe)]
        subprocess.run(cmd,cwd=ROOT,check=True,capture_output=True,text=True)
        (cls.work/'frontend').mkdir()
        for name in ('index.html','style.css','script.js'):
            (cls.work/'frontend'/name).write_bytes((ROOT/'frontend'/name).read_bytes())
        cls.proc=subprocess.Popen([str(cls.exe)],cwd=cls.work,env={**os.environ,'PLANNER_PORT':'18081'},stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        for _ in range(30):
            try:
                urllib.request.urlopen(BASE+'/api/state',timeout=0.5)
                break
            except Exception:
                time.sleep(0.1)
        else:
            raise RuntimeError('Server failed to start. Is port 8080 in use?')

    @classmethod
    def tearDownClass(cls):
        cls.proc.terminate()
        try: cls.proc.wait(timeout=3)
        except subprocess.TimeoutExpired: cls.proc.kill()
        cls.tmp.cleanup()

    def request(self,path,**values):
        payload=urllib.parse.urlencode(values).encode() if values else None
        if not values and path != '/api/state': payload=b''
        request=urllib.request.Request(BASE+path,data=payload) if payload is not None else BASE+path
        with urllib.request.urlopen(request,timeout=3) as response:
            return json.loads(response.read().decode())

    def test_01_frontend_served(self):
        with urllib.request.urlopen(BASE+'/') as response:
            html=response.read().decode()
            self.assertIn('Smart Study Planner',html)
        with urllib.request.urlopen(BASE+'/script.js') as response:
            self.assertIn('async function api',response.read().decode())

    def test_02_empty_state(self):
        state=self.request('/api/state')
        self.assertTrue(state['ok'])
        self.assertEqual(len(state['tasks']),0)
        self.assertEqual(state['dailyHours'],3)

    def test_03_validation(self):
        bad=self.request('/api/add',name='Invalid',importance=7,deadline=1,hours=3)
        self.assertFalse(bad['ok'])
        self.assertEqual(len(self.request('/api/state')['tasks']),0)

    def test_04_add_priority_conflicts(self):
        self.request('/api/add',name='DSA assignment',importance=3,deadline=1,hours=3)
        s=self.request('/api/add',name='OOP lab',importance=2,deadline=2,hours=4)
        self.assertEqual(len(s['tasks']),2)
        self.assertEqual(s['priority'][0]['name'],'DSA assignment')
        self.assertEqual(len(s['conflicts']),1)
        self.assertEqual(s['conflicts'][0]['deadline'],2)
        self.assertEqual(s['conflicts'][0]['shortage'],1)

    def test_05_recovery_consistency(self):
        s=self.request('/api/recovery',missed=2)
        self.assertEqual(s['missedToday'],2)
        self.assertEqual(s['plan']['conflicts'],s['conflicts'])
        self.assertEqual(s['plan']['unscheduled'],3)
        self.assertTrue(any(x['risk']=='HIGH' for x in s['risks']))

    def test_06_whatif_does_not_mutate(self):
        before=self.request('/api/state')
        scenario=self.request('/api/whatif',hours=5,missed=0)
        after=self.request('/api/state')
        self.assertEqual(before,after)
        self.assertEqual(scenario['scenario']['unscheduled'],0)
        self.assertEqual(scenario['scenario']['conflicts'],[])

    def test_07_progress_and_persistence(self):
        s=self.request('/api/progress',id=2,hours=1)
        self.assertEqual(s['tasks'][1]['remaining'],3)
        self.assertTrue((self.work/'planner_data.txt').exists())

    def test_08_next_day(self):
        s=self.request('/api/next')
        self.assertEqual(s['day'],2)
        self.assertEqual(s['missedToday'],0)
        self.assertEqual(s['tasks'][0]['deadline'],0)

    def test_09_removal(self):
        s=self.request('/api/remove',id=1)
        self.assertEqual(len(s['tasks']),1)
        missing=self.request('/api/progress',id=1,hours=1)
        self.assertFalse(missing['ok'])

if __name__=='__main__':unittest.main(verbosity=2)
