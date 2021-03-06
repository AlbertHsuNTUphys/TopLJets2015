import ROOT
import sys
import os
import pickle
import re
from prepareOptimScanCards import OPTIMLIST
from TopLJets2015.TopAnalysis.Plot import *

def getLimitsFrom(url,getObs=False):

    """parses the r95 limits from the tree"""

    fIn=ROOT.TFile.Open(url)
    try:
        t=fIn.Get('limit')
        vals=[]
        if getObs:
            t.GetEntry(t.GetEntriesFast()-1)
            vals=[t.limit]*5
        else:
            for i in range(5):
                t.GetEntry(i)
                vals.append(t.limit)
        fIn.Close()
    except:
        vals=[999.]*5

    return [vals[2],vals[3]-vals[2],vals[1]-vals[2],vals[4]-vals[2],vals[0]-vals[2]]


def getSignificanceFrom(url):

    """parses the significance value from combine tree and converts it to a p-value"""

    fIn=ROOT.TFile.Open(url)
    try:
        t=fIn.Get('limit')
        t.GetEntry(0)
        vals=[t.limit,ROOT.RooStats.SignificanceToPValue(t.limit)]
        fIn.Close()
    except:
        vals=[0,1]

    return vals


def showOptimizationScan(results,name,title,mass,lumi):

    """a bar plot with the expected limits obtained at each optimization point"""

    c=ROOT.TCanvas('c','c',500,800)
    c.SetLeftMargin(0.3)
    c.SetRightMargin(0.05)
    c.SetTopMargin(0.05)
    c.SetBottomMargin(0.15)
    c.SetGridx()
    c.SetGridy()

    h    = ROOT.TH1F('optimscan',    '#sigma_{fid}^{95%CL};;95% CL on #sigma_{fid} [pb]', len(OPTIMLIST),0,len(OPTIMLIST))
    hsig = ROOT.TH1F('optimscansig', 'Significance;;Significance (asymptotic)',           len(OPTIMLIST),0,len(OPTIMLIST))
    maxy=0
    for ipt,limits in results:

        kinCuts,rpCuts,categs=OPTIMLIST[ipt-1]
        xlabel=kinCuts[0] if '_z' in name else kinCuts[1] #dirty hack
        xlabel=xlabel.replace('bosonpt','V')
        xlabel+=',' + rpCuts
        xlabel=xlabel.replace('csi1','#xi')
        xlabel=xlabel.replace('&& csi2>','/')
        xlabel=xlabel.replace('l1pt','l')
        xlabel=xlabel.replace('&& l2pt>','/ ')
        xlabel=xlabel.replace(' && ',',')
        xlabel += ' (' + categs.split(',')[0] + ')'
        xlabel=xlabel.replace('nvtx','vtx')
        xlabel=xlabel.replace(' ','')
        xlabel='#scale[0.8]{%s}'%xlabel
        h.GetXaxis().SetBinLabel(ipt,xlabel)
        h.SetBinContent(ipt,limits[0])

        hsig.GetXaxis().SetBinLabel(ipt,xlabel)
        hsig.SetBinContent(ipt,limits[5])

        maxy=max(maxy,limits[0])

    h.SetFillStyle(1001)
    h.SetFillColor(ROOT.kGray)
    h.GetYaxis().SetLabelSize(0.025)
    h.GetYaxis().SetTitleSize(0.03)
    h.GetYaxis().SetTitleOffset(0.8)
    h.GetYaxis().SetRangeUser(0,1.1*maxy)
    h.GetYaxis().SetNdivisions(5)
    h.Draw('hbar')

    #scale hint1 to the pad coordinates
    rightmax = 1.1*hsig.GetMaximum()
    scale = h.GetMaximum()/rightmax
    hsig.SetLineColor(ROOT.kOrange+2)
    hsig.SetFillColor(ROOT.kOrange+2)
    hsig.SetFillStyle(0)
    hsig.Scale(scale)
    hsig.Draw("hbarsame")

    leg=ROOT.TLegend(0.7,0.92,0.95,0.83)
    leg.SetTextFont(42)
    leg.SetBorderSize(0)
    leg.SetTextSize(0.035)
    leg.SetHeader(title+'(%d)'%mass)
    leg.AddEntry(h,h.GetTitle(),'f')
    leg.AddEntry(hsig,hsig.GetTitle(),'f')
    leg.Draw()

    axis = ROOT.TGaxis(0,-2, h.GetMaximum(),-2, 0, rightmax,5,"-L=");
    axis.SetTickLength(0.005)
    axis.SetTextFont(42)
    axis.SetLabelFont(42)
    axis.SetLabelOffset(-0.05)
    axis.SetLabelSize(0.03)
    axis.SetTitle(hsig.GetYaxis().GetTitle())
    axis.SetTitleOffset(0.8)
    axis.SetTextSize(0.001)
    axis.Draw()

    tex=ROOT.TLatex()
    tex.SetTextFont(42)
    tex.SetTextSize(0.04)
    tex.SetNDC()
    tex.DrawLatex(0.3,0.96,'#bf{CMS} #it{preliminary}')
    tex.SetTextAlign(ROOT.kHAlignRight+ROOT.kVAlignBottom)
    tex.DrawLatex(0.95,0.96,'#scale[0.8]{%s fb^{-1} (13 TeV)}'%lumi)

    c.Modified()
    c.Update()
    c.RedrawAxis()
    for ext in ['png','pdf']:
        c.SaveAs('%s.%s'%(name,ext))


def showLimits(results,name,title,lumi,results_obs=None):

    """the brazilian flag plot for the limits"""

    c=ROOT.TCanvas('c','c',500,500)
    c.SetLeftMargin(0.12)
    c.SetRightMargin(0.03)
    c.SetTopMargin(0.05)
    c.SetBottomMargin(0.1)

    r68=ROOT.TGraphAsymmErrors()
    r68.SetFillColor(8)
    r68.SetFillStyle(1001)
    r68.SetMarkerColor(8)
    r68.SetLineColor(1)
    r68.SetLineWidth(2)
    r68.SetLineStyle(7)
    r68.SetName('r68')
    r68.SetTitle('Expected #pm1#sigma')

    r95=ROOT.TGraphAsymmErrors()
    r95.SetFillColor(5)
    r95.SetFillStyle(1001)
    r95.SetMarkerColor(5)
    r95.SetLineColor(1)
    r95.SetLineWidth(2)
    r95.SetLineStyle(7)
    r95.SetName('r95')
    r95.SetTitle('Expected  #pm2#sigma')

    rmed=ROOT.TGraph()
    rmed.SetName('rmed')
    rmed.SetFillColor(0)
    rmed.SetFillStyle(0)
    rmed.SetLineWidth(2)
    rmed.SetLineStyle(7)
    rmed.SetLineColor(1)

    maxRan=0
    for m,limits in results:
        ipt=rmed.GetN()
        rmed.SetPoint(ipt,m,limits[0])
        r68.SetPoint(ipt,m,limits[0])
        r68.SetPointError(ipt,10,10,abs(limits[2]),abs(limits[1]))
        r95.SetPoint(ipt,m,limits[0])
        r95.SetPointError(ipt,10,10,abs(limits[4]),abs(limits[3]))
        
        maxRan=max(maxRan,limits[0]+limits[3])

    r68.Sort()
    r95.Sort()
    rmed.Sort()

    #observed results
    robs=None
    if results_obs:
        robs=ROOT.TGraph()
        robs.SetName('robs')
        robs.SetTitle('Observed')
        robs.SetFillColor(0)
        robs.SetFillStyle(0)
        robs.SetLineColor(1)
        robs.SetLineWidth(3)
        robs.SetLineStyle(1)
        robs.SetMarkerStyle(20)
        robs.SetMarkerColor(1)

        for m,limits in results_obs:
            ipt=robs.GetN()
            robs.SetPoint(ipt,m,limits[0])
            maxRan=max(maxRan,limits[0])

        robs.Sort()
    
    mg=ROOT.TMultiGraph()
    mg.Add(r95,'l3')
    mg.Add(r68,'l3')
    mg.Add(rmed,'lp')
    if robs:
        mg.Add(robs,'lp')

    frame=ROOT.TH1F('frame',';m_{X} [GeV];95% CL limits on #sigma_{fid}#timesBR [pb]',1,580,1700)
    frame.GetYaxis().SetRangeUser(5e-3,5)
    frame.SetBinContent(1,1)
    frame.SetLineWidth(2)
    frame.SetLineColor(ROOT.kRed)
    frame.Draw('hist')
    mg.Draw('l3')

    leg=ROOT.TLegend(0.15,0.92,0.4,0.68)
    leg.SetTextFont(42)
    leg.SetTextSize(0.035)
    leg.SetBorderSize(0)
    leg.SetFillStyle(0)
    if robs:
        leg.AddEntry(robs,robs.GetTitle(),'l')
    leg.AddEntry(r68,r68.GetTitle(),'lf')
    leg.AddEntry(r95,r95.GetTitle(),'lf')
    leg.Draw()

    tex=ROOT.TLatex()
    tex.SetTextFont(42)
    tex.SetTextSize(0.04)
    tex.SetNDC()
    tex.DrawLatex(0.12,0.96,'#bf{CMS} #it{preliminary}')
    tex.SetTextAlign(ROOT.kHAlignRight+ROOT.kVAlignCenter)
    tex.DrawLatex(0.97,0.975,'#scale[0.9]{%s fb^{-1} (13 TeV)}'%lumi)
    tex.DrawLatex(0.95,0.9,title)
    c.Modified()
    c.Update()   
    c.RedrawAxis()
    
    for ext in ['png','pdf','root']:
        c.SaveAs('%s.%s'%(name,ext))
    
    c.SetLogy()
    c.Modified()
    c.Update()   
    c.RedrawAxis()

    for ext in ['png','pdf']:
        c.SaveAs('%s_log.%s'%(name,ext))


def showShapes(resultsDir,name,title,mass,boson,r95,sig,lumi,plotData=True,showPseudoData=True,showAllBkgs=True,subCatTitles=None):

    """ show the shapes corresponding to a given analysis """
    
    shapeFiles=[os.path.join(resultsDir,f) for f in os.listdir(resultsDir) if 'shapes_' in f and '.root' in f]
    for f in shapeFiles:

        try:
            angle=int(re.search('_a(\d+)',f).group(1))
        except:
            print 'No angle could be parsed, assuming this is an inclusive analysis'
            angle=None

        fIn=ROOT.TFile.Open(f)

        #check consistency of the file with the boson to plot
        if 'shapes_22' in f and boson=='g':
            v='g'
            channel='#gamma'
        elif 'shapes_169' in f and boson=='z': 
            v='zmm'
            channel='Z#rightarrow#mu#mu'
        elif 'shapes_121' in f and boson=='z': 
            v='zee'
            channel='Z#rightarrowee'
        else:
            continue
        
        ncats=len(subCatTitles) if subCatTitles else 1
        for icat in range(ncats):
            
            if angle>=0:
                bkgH       = fIn.Get('bkg_%s_a%d_%d'%(v,angle,icat))
                fidsigH    = fIn.Get('fidsig_%s_a%d_%d_m%d'%(v,angle,icat,mass))
                outfidsigH = fIn.Get('outfidsig_%s_a%d_%d_m%d'%(v,angle,icat,mass))
                dataH = fIn.Get('data_obs_%s_a%d_%d'%(v,angle,icat))
            else:
                bkgH       = fIn.Get('bkg_%s_%d'%(v,icat))
                fidsigH    = fIn.Get('fidsig_%s_%d_m%d'%(v,icat,mass))
                outfidsigH = fIn.Get('outfidsig_%s_%d_m%d'%(v,icat,mass))
                dataH      = fIn.Get('data_obs_%s_%d'%(v,icat))

            print bkgH,fidsigH,outfidsigH,dataH

            #try:
            #    fidsigH.Scale(5)
            #    outfidsigH.Scale(5)
            #except:
            #    fidsigH=None
            #    outfidsigH=None


            if showPseudoData:
                dataH.Reset('ICE')
                nexp=bkgH.Integral()
                if fidsigH: nexp+=bkgH.Integral()
                if outfidsigH: nexp+=outfidsigH.Integral()
                for iev in range( ROOT.gRandom.Poisson( nexp ) ):
                    dataH.Fill( bkgH.GetRandom() )
            try:
                
                #main shapes
                if angle>=0:
                    p=Plot('%s_%s_a%d_cat%d'%(name,v,angle,icat))
                else:
                    p=Plot('%s_%s_inc_cat%d'%(name,v,icat))

                p.xtit='Missing mass [GeV]'
                p.ytit='Events'
                if fidsigH:
                    #p.add(fidsigH,            title='#scale[0.5]{5x}'+title+'#scale[0.8]{(%d)}'%mass, color=ROOT.TColor.GetColor('#fdc086'), isData=False, spImpose=False, isSyst=False)
                    p.add(fidsigH,            title='fiducial #scale[0.8]{(%d)}'%mass, color=ROOT.TColor.GetColor('#fdc086'), isData=False, spImpose=False, isSyst=False)
    
                if showAllBkgs:
                    if outfidsigH:
                        #p.add(outfidsigH,         title='#scale[0.5]{5x}non-fiducial',  color=ROOT.TColor.GetColor('#a6cee3'), isData=False, spImpose=False, isSyst=False)
                        p.add(outfidsigH,         title='non-fiducial',  color=ROOT.TColor.GetColor('#a6cee3'), isData=False, spImpose=False, isSyst=False)
                    p.add(bkgH,               title='background',    color=ROOT.TColor.GetColor('#1f78b4'), isData=False, spImpose=False, isSyst=False)
                else:
                    allBkg=bkgH.Clone('allbkg')
                    if outfidsigH : 
                        allBkg.Add(outfidsigH)
                    p.add(allBkg,               title='background',    color=ROOT.TColor.GetColor('#1f78b4'), isData=False, spImpose=False, isSyst=False)

                if plotData:
                    dtitle='pseudo-data' if showPseudoData else 'Data'
                    p.add(dataH, title=dtitle,   color=1, isData=True, spImpose=False, isSyst=False)
                if fidsigH:
                    p.add(fidsigH.Clone(),    title=title+'#scale[0.8]{(%d)}'%mass, color=ROOT.TColor.GetColor('#fdc086'), isData=False, spImpose=True,  isSyst=False)

                if showAllBkgs and outfidsigH:
                    p.add(outfidsigH.Clone(), title='non-fiducial',  color=ROOT.TColor.GetColor('#a6cee3'), isData=False, spImpose=True,  isSyst=False)
                p.ratiorange=[0.68,1.32]

                extraText=[subCatTitles[icat] if subCatTitles else '']
                if r95: extraText += ['#mu_{95}(exp.)<%3.3f'%r95]
                if sig: extraText += ['S(exp.)=%3.3f'%sig]
                p.show('./',lumi*1000,extraText='\\'.join(extraText))

                colors=[ROOT.kGreen+1,ROOT.kAzure+3,ROOT.kRed+2]

                #background systs
                if angle>=0:
                    p=Plot('%s_%s_a%d_cat%d_bkgunc'%(name,v,angle,icat))
                else:
                    p=Plot('%s_%s_inc_cat%d_bkgunc'%(name,v,icat))
                p.noErrorsOnRatio=True
                p.doPoissonErrorBars=False
                p.xtit='Missing mass [GeV]'
                p.ytit='Events'
                p.add(bkgH, title='background', color=1, isData=True,spImpose=False, isSyst=False)
                ic=0
                for syst,title in [('Up','Pileup spec.'),
                                   ('SingleDiffUp','Single diff.')]:
                    if angle>=0:
                        p.add(fIn.Get('bkg_%s_a%d_%d_bkgShape%s'%(v,angle,icat,syst)), title=title, color=colors[ic], isData=False, spImpose=False, isSyst=False)
                    else:
                        p.add(fIn.Get('bkg_%s_inc_%d_bkgShape%s'%(v,icat,syst)), title=title, color=colors[ic], isData=False, spImpose=False, isSyst=False)
                    ic+=1
                p.ratiorange=[0.76,1.24]
                if angle>=0:
                    extraText='%s, %d#murad'%(channel,angle)
                else:
                    extraText='%s, inclusive'%(channel)
                p.show('./',lumi,noStack=True,extraText=extraText)

                #signal systs
                if fidsigH:
                    if angle>=0:
                        p=Plot('%s_%s_a%d_cat%d_sigunc'%(name,v,angle,icat))
                    else:
                        p=Plot('%s_%s_inc_cat%d_sigunc'%(name,v,icat))
                    #fidsigH.Scale(1./5.)
                    p.doPoissonErrorBars=False
                    p.noErrorsOnRatio=True
                    p.xtit='Missing mass [GeV]'
                    p.ytit='Events'
                    p.add(fidsigH, title='signal', color=1, isData=True,spImpose=False, isSyst=False)
                    ic=0
                    for syst,title in [('ShapeUp','Pileup spec.'),
                                       ('CalibUp','Time dependence'),
                                       ('PzModelUp','p_{z}(pp)')]:
                        if angle>=0:
                            p.add(fIn.Get('fidsig_%s_a%d_%d_m%d_sig%s'%(v,angle,icat,mass,syst)), title=title, color=colors[ic], isData=False, spImpose=False, isSyst=False)
                        else:
                            p.add(fIn.Get('fidsig_%s_%d_m%d_sig%s'%(v,icat,mass,syst)), title=title, color=colors[ic], isData=False, spImpose=False, isSyst=False)
                        ic+=1
                    p.ratiorange=[0.76,1.24]
                    if angle>=0:
                        extraText='%s, %d#murad\\m_{X}=%d GeV'%(channel,angle,mass)
                    else:
                        extraText='%s, inclusive\\m_{X}=%d GeV'%(channel,mass)
                    p.show('./',lumi,noStack=True,extraText=extraText)

                #out fiducial signal systs
                if outfidsigH:
                    if angle>=0:
                        p=Plot('%s_%s_a%d_cat%d_outfidsigunc'%(name,v,angle,icat))
                    else:
                        p=Plot('%s_%s_inc_cat%d_outfidsigunc'%(name,v,icat))
                    #outfidsigH.Scale(1./5.)
                    p.doPoissonErrorBars=False
                    p.noErrorsOnRatio=True
                    p.xtit='Missing mass [GeV]'
                    p.ytit='Events'
                    p.add(outfidsigH, title='out-fid. signal', color=1, isData=True,spImpose=False, isSyst=False)
                    ic=0
                    for syst,title in [('ShapeUp','Pileup spec.'),
                                       ('CalibUp','Time dependence'),
                                       ('PzModelUp','p_{z}(pp)')]:
                        if angle>=0:
                            p.add(fIn.Get('outfidsig_%s_a%d_%d_m%d_sig%s'%(v,angle,icat,mass,syst)), title=title, color=colors[ic], isData=False, spImpose=False, isSyst=False)
                        else:
                            p.add(fIn.Get('outfidsig_%s_%d_m%d_sig%s'%(v,icat,mass,syst)), title=title, color=colors[ic], isData=False, spImpose=False, isSyst=False)
                        ic+=1

                    p.ratiorange=[0.76,1.24]
                    if angle>=0:
                        extraText='%s, %d#murad\\m_{X}=%d GeV'%(channel,angle,mass)
                    else:
                        extraText='%s, inclusive\\m_{X}=%d GeV'%(channel,mass)
                    p.show('./',lumi,noStack=True,extraText=extraText)


            except Exception as e:
                print e
                pass

        fIn.Close()



def main():

    ROOT.gStyle.SetOptStat(0)
    ROOT.gStyle.SetOptTitle(0)
    ROOT.gROOT.SetBatch(True)

    #loop over all results in output directory
    baseDir=sys.argv[1]
    pickOptimPt=None
    plotPFix=''
    if len(sys.argv)>2:
        pickOptimPt=int(sys.argv[2])
        plotPFix='_pt%d'%pickOptimPt
    showPseudoData=True
    if len(sys.argv)>3:
        showPseudoData=bool(sys.argv[3])
   
    results={'z':{},'g':{}}
    toCheck=[]
    for optimPt in os.listdir(baseDir):

        try:
            pt = int(re.search('optim_(\d+)', optimPt).group(1))
        except:
            print optimPt,'doesn\'t look like an optimization directory...bailing out'
            continue

        #cherry pick a given optimPt
        if pickOptimPt and pt!=pickOptimPt: continue

        optimDir=os.path.join(baseDir,optimPt)
        limitFiles=[os.path.join(optimDir,x) for x in os.listdir(optimDir) if 'PPzX.AsymptoticLimits' in x]

        for f in limitFiles:
            mass = int(re.search('.mH(\d+)', f).group(1))
        
            if not mass in results['z']:
                results['z'][mass]=[]
                results['g'][mass]=[]            
        
            resultVals=getLimitsFrom(f)+getSignificanceFrom(f.replace('AsymptoticLimits','Significance'))
            if resultVals[0]<900:
                results['z'][mass].append( (pt,resultVals) )
            else:
                toCheck.append( (f,pt) )

            fg=f.replace('PPzX','PPgX')
            resultVals=getLimitsFrom(fg)+getSignificanceFrom(fg.replace('AsymptoticLimits','Significance'))
            if resultVals[0]<900:
                results['g'][mass].append( (pt,resultVals) )
            else:
                toCheck.append( (fg,pt) )


    #print results and show limit plots
    fOut=open('optimresults.dat','w')
    for v in results.keys():

        #cosmetics for the plots
        title='pp#rightarrowppZX'
        lumi=37.5
        if v=='g':
            title='pp#rightarrowpp#gammaX'
            lumi=2.64

        fOut.write('-'*50+'\n')
        fOut.write('%s @ lumi=%f\n'%(title,lumi))
    
        bestResults=[]
        bestResultsDir=[]
        for m in results[v]:

            results[v][m].sort(key=lambda x: x[1], reverse=False)
            bestPoint,limits=results[v][m][0]
        
            if m in [800,1000,1200,1400]:
                showShapes( resultsDir=os.path.join(baseDir,'optim_%d'%bestPoint),
                            name='shapes_m%d%s'%(m,plotPFix),
                            title=title,
                            mass=m,
                            boson=v,
                            plotData=True,
                            showPseudoData=False,
                            r95=limits[0],
                            sig=limits[5],
                            lumi=lumi )

            kinCuts,rpCuts,categs=OPTIMLIST[bestPoint-1]
            fOut.write('%10s r95<%3.3f S=%3.3f (p-val=%3.3f) %25s %25s %25s %d\n'%(str(m),limits[0],limits[5],limits[6],kinCuts[0] if v=='z' else kinCuts[1] ,rpCuts,categs,bestPoint))
            bestResults.append( (m,limits) )

            showOptimizationScan(results[v][m],'optimscan_%s_m%d%s'%(v,m,plotPFix),title,m,lumi)

        showLimits(bestResults,'limits_%s%s'%(v,plotPFix),title,lumi)
    
    if len(toCheck)>0:
        fOut.write('Some points are fishy - please check:')
        for f,pt in toCheck:
            fOut.write('\t %d %s\n'%(pt,f))

    fOut.close()
    
    print 'Summary (and list of files to check) can be found in optimresults.dat'

    


if __name__ == "__main__":
    main()
