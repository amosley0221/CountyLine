"""Original supporting models; Pruitt palette/silhouette follows IMG_1934.
Uses the existing rig, without re-exporting Reed or Salazar.
"""
import sys, runpy
from pathlib import Path
sys.argv.append('--definitions-only')
g=runpy.run_path(str(Path(__file__).with_name('author_period_characters.py')))
globals().update({k:v for k,v in g.items() if not k.startswith('__')})
for kind in ('Pruitt','NorthLaneResident'):
 f=Figure('SK_'+kind+'_Period'); deputy=kind=='Pruitt'
 shirt='Shirt' if deputy else 'Linen'; pants='Shirt' if deputy else 'Trouser'
 f.torso([(1.00,.145,.102,.025),(1.10,.15,.102,.025),(1.30,.17,.103,.03),(1.48,.19,.084,.04),(1.56,.07,.054,.04),(1.585,.05,.049,.04)],shirt)
 f.torso([(.90,.09,.084,.025),(.97,.147,.102,.025),(1.02,.15,.104,.025),(1.065,.145,.10,.025)],pants,weights={'pelvis':1})
 f.torso([(1.035,.152,.106,.025),(1.072,.147,.104,.025)],'Leather',weights={'pelvis':1})
 f.face([(-.023,-.083,1.07),(.023,-.083,1.07),(.023,-.087,1.036),(-.023,-.087,1.036)],'Brass',{'pelvis':1})
 if not deputy:
  f.torso([(1.08,.155,.109,.025),(1.26,.177,.109,.03),(1.46,.19,.094,.038),(1.52,.10,.065,.04)],'Waistcoat',.12)
 for s in (-1,1):
  f.face([(s*.008,-.067,1.54),(s*.069,-.063,1.523),(s*.046,-.088,1.46)],shirt,{'spine_03':1})
  if deputy:
   f.face([(s*.065,-.087,1.385),(s*.15,-.065,1.385),(s*.15,-.075,1.29),(s*.065,-.096,1.29)],shirt,body_weight)
   f.tube([(s*.065,-.09,1.375),(s*.15,-.071,1.375)],[.002,.002],'HatFelt',body_weight,6)
   f.ellipsoid((s*.107,-.091,1.36),(.004,.003,.004),'Brass','spine_02',8,4)
 for z in (1.11,1.2,1.29,1.38):f.ellipsoid((0,-.083,z),(.004,.004,.004),'HatBand','spine_02',8,4)
 if deputy:
  f.ellipsoid((0,-.068,1.495),(.017,.014,.018),'Hair','spine_03',10,5)
  f.face([(-.011,-.084,1.48),(.011,-.084,1.48),(.02,-.085,1.19),(0,-.088,1.165),(-.02,-.085,1.19)],'Hair',body_weight)
  pts=[]
  for i in range(12):
   a=math.tau*i/12;r=.023 if i%2==0 else .013
   pts.append((.108+math.sin(a)*r,-.082,1.417+math.cos(a)*r))
  f.face(list(reversed(pts)),'Brass',{'spine_03':1})
 for side in ('l','r'):
  upper,elbow,hand=[bones[n+'_'+side] for n in ('upperarm','lowerarm','hand')]
  def sw(p,a=upper,b=elbow,c=hand,s=side):
   axis=(c-a).normalized();prox=(p-a).dot(axis)
   if prox<0:
    blend=min(1,-prox/.10);return {'spine_03':blend,'upperarm_'+s:1-blend}
   u=max(0,min(1,.5+(p-b).dot(axis)/.12));return {'upperarm_'+s:1-u,'lowerarm_'+s:u}
  f.tube([upper.lerp(bones['clavicle_'+side],.75),upper,upper.lerp(elbow,.4),elbow,hand],[.035,.069,.065,.051,.034],shirt,sw,20)
  f.tube([hand.lerp(elbow,.12),hand],[.037,.035],shirt,{'lowerarm_'+side:1},16)
  thigh,knee,foot=[bones[n+'_'+side] for n in ('thigh','calf','foot')]
  def lw(p,k=knee,s=side):
   u=max(0,min(1,.5+(k.z-p.z)/.16));return {'thigh_'+s:1-u,'calf_'+s:u}
  f.tube([thigh,thigh.lerp(knee,.3),knee,knee.lerp(foot,.85),foot],[.091,.087,.064,.054,.052],pants,lw,18)
  f.ellipsoid((foot.x,foot.y-.055,.065),(.068,.15,.058),'Leather','foot_'+side,18,8)
  f.ellipsoid((foot.x,foot.y-.052,.023),(.07,.15,.017),'HatBand','foot_'+side,18,6)
  f.palm(hand,bones['middle_01_'+side],bones['index_01_'+side],bones['pinky_01_'+side],'hand_'+side)
  for finger in ('thumb','index','middle','ring','pinky'):
   names=[finger+'_'+str(i).zfill(2)+'_'+side for i in (1,2,3)];pts=[bones[n] for n in names]
   for j in range(2):f.tube([pts[j],pts[j+1]],[.009,.008],'Skin',{names[j]:1},8)
   f.tube([pts[2],pts[2]+(pts[2]-pts[1])*.7],[.008,.0055],'Skin',{names[2]:1},8)
 f.tube([(0,.04,1.52),(0,.04,1.66)],[.044,.046],'Skin',{'neck_01':1})
 f.torso([(1.635,.042,.046,.017),(1.655,.06,.058,.018),(1.69,.076,.071,.024),(1.745,.079,.075,.028),(1.795,.072,.07,.034),(1.825,.05,.052,.035),(1.84,.001,.001,.035)],'Skin',weights={'head':1},n=40)
 for s in (-1,1):
  f.ellipsoid((s*.079,.026,1.735),(.013,.015,.027),'Skin','head')
  f.ellipsoid((s*.031,-.042,1.744),(.015,.005,.0045),'EyeWhite','head')
  f.ellipsoid((s*.031,-.047,1.744),(.0035,.002,.0035),'Iris','head')
  f.ellipsoid((s*.032,-.042,1.76),(.021,.004,.0035),'Hair','head')
 f.ellipsoid((0,-.057,1.723),(.012,.022,.022),'Skin','head')
 f.ellipsoid((0,-.052,1.684),(.025,.006,.003),'Mouth','head')
 f.torso([(1.787,.079,.078,.035),(1.82,.065,.064,.035),(1.85,.025,.036,.035),(1.852,.001,.001,.035)],'Hair',weights={'head':1})
 if deputy:
  f.ellipsoid((-.025,-.012,1.825),(.047,.06,.022),'Hair','head',20,8)
 else:
  f.ellipsoid((0,-.052,1.7),(.029,.005,.006),'Hair','head',20,6)
  f.ellipsoid((0,.018,1.829),(.102,.115,.041),'HatFelt','head',24,10)
  f.ellipsoid((0,-.066,1.812),(.094,.083,.007),'HatBand','head',24,6)
 ob=f.finish();ob.hide_set(True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'SupportingCast.blend'))
print('CL_SUPPORTING_CAST_AUTHORED')
