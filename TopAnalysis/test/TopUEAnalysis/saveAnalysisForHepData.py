import os
import pickle
import ROOT
import sys

plotList=[
    ('chmult/inc',     'd01-x01-y01'),
    ('chflux/inc',     'd01-x02-y01'),
    ('chfluxz/inc',    'd01-x03-y01'),
    ('chavgpt/inc',    'd01-x04-y01'),
    ('chavgpz/inc',    'd01-x05-y01'),
    ('sphericity/inc', 'd01-x06-y01'),
    ('aplanarity/inc', 'd01-x07-y01'),
    ('C/inc',          'd01-x08-y01'),
    ('D/inc',          'd01-x09-y01')
    ]

baseDir='/afs/cern.ch/user/p/psilva/work/Top/CMSSW_8_0_28/src/TopLJets2015/TopAnalysis/store/TOP-17-015'
outDir='./'
if len(sys.argv)>1:  baseDir=sys.argv[1]
if len(sys.argv)>2: outDir=sys.argv[2]

fOut={'data' : open(os.path.join(outDir,'CMS_2017_TOP_17_015.yoda'),'w'),
      'mc'   : open(os.path.join(outDir,'CMS_2017_TOP_17_015_MC.yoda'),'w')}

for p,pname in plotList:

    #readout data
    gr={'data':None,'mc':None}
    with open(os.path.join(baseDir,p,'unfold/unfold_summary.pck'),'r') as cachefile:
        uePlots=pickle.load(cachefile)
        if 'data' in uePlots   : gr['data']=uePlots['Data'].plot[0]
        if 'PW+PY8' in uePlots : gr['mc']=uePlots['PW+PY8'].plot[1][0] #stat unc. only

    #dump plot in yoda format
    for t in ['data','mc']:

        if not gr[t]:
            print 'Skipping',p,'for',t
            continue

        fOut[t].write('BEGIN YODA_SCATTER2D /CMS_2017_TOP_17_015/%s\n'%pname)
        fOut[t].write('Path=/CMS_2017_TOP_17_015/%s\n'%pname)
        fOut[t].write('Type=Scatter2D\n')
        fOut[t].write('# xval xerr- xerr+ yval yerr- yerr+\n')
        x,xref,y=ROOT.Double(0),ROOT.Double(0),ROOT.Double(0)
        print gr[t],gr[t].GetN()
        for i in xrange(0,gr[t].GetN()):
            #mc has no error and was probably shifted: use data (if available)
            grRef=gr['data'] if gr['data'] else gr[t]
            grRef.GetPoint(i,xref,y)
            exhi,exlo=grRef.GetErrorXhigh(i),grRef.GetErrorXlow(i) 
            
            #take y values and error from model/data
            gr[t].GetPoint(i,x,y)
            eyhi,eylo=gr[t].GetErrorYhigh(i),gr[t].GetErrorYlow(i),
            fOut[t].write('%.6g %.6g %.6g %.6g %.6g %.6g\n'%(float(xref),exlo,exhi,float(y),eylo,eyhi))
        fOut[t].write('END YODA_SCATTER2D\n')
        fOut[t].write('\n')

#close open files
for t in ['data','mc']: 
    fOut[t].write('\n\n# this file was generated from %s\n'%t)
    fOut[t].close()
