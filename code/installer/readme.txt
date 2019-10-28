  28 October 2019 3.6.1089.3
- Eventor API address defaulted to Swedish Eventor
- Improve detection of required Microsoft run-time libraries

  15 September 2019
	Merge with MEOS 3.6.1089

  14 October 2018 3.5.916.0
	Merge with MEOS 3.5.916

  23 March 2018 3.4.717.7
- XML results export for IOF XML 2.0.3 had broken split times
- Use Eventor provided shortnames for club short names
	
	11 March 2018 3.4.717.6
- XML import unified classes incorrectly, eg M21A and M21AS.

  10 March 2018
- Handle defective XML import (contained "Leg" info for classes that
  weren't relays). This is an improvement on fix from 9th March 2018.

  9 March 2018
- Zero time set incorrectly when events were loaded from Eventor.
- Start time was set to zero time for all classes (ie masss start) when
  events were loaded from Eventor.

  15 November 2017
- MEOS crashed when an unknown runner downloaded (bug introduced 3.4.717.2)

  6 November 2017
- Allow specification of a "series" for SSS style events rather than forcing "sss"

  1 November 2017
- Set SSS classes when comp (without classes) is created before pressing SSS button.
- Set the class for a competitor from gender/age if their details are retrieved at
  download from runner db.   
  
  23 October 2017
- Merge with standard MEOS 3.4.717
- Add support for export of Score points in IOF XML 3
- Add support for instant SSS set-up when event has been imported from Eventor 

  8 February 2017
- Merge with standard MEOS 3.4.690 (3.4 RC1)
  For changes see 3.4 RC1 notes in http://www.melin.nu/meos/en/whatsnew25.php
- This fixes problem introduced in 3.4.666 with tiny font on splits print.
- This fixes problem introduced where Competitors in Forest found nobody.
  
  18 December 2016 3.4.666.1
- Merge with standard MEOS 3.4.666
  For changes see 3.4 Beta 1 notes in http://www.melin.nu/meos/en/whatsnew25.php
- Remove MEOS-OZ random cross-class draw (MEOS now has this)
- Remove MEOS-OZ Competitors in Forest logic
  
  24 November 2016 3.3.526.7
- Stop creating duplicate custom reports for SSS.

  17 November 2016 3.3.526.6
- Set age, gender details in SSS classes.
- Set class if possible in quick entry mode.
- Remove SSS start list load functionality.

  23 October 2016 3.3.526.5
- Look for installed template for SSS before looking on web.
- SSS template update to allow quick-entry on all courses.
- Remove junk clubs from clubs database.

  8 October 2016 3.3.526.4
- Delete competition did nothing. Now fixed.

  19 March 2016 3.3.526.3
- Add new & altered club names 

  23 February 2016 3.3.526.2
- Intelligent processing of known rented cards when using quick entry 
 
 9 February 2016 3.3.526
- Merge with standard MEOS 3.3.526.1
  For changes see 3.3 Beta 1 notes in http://www.melin.nu/meos/en/whatsnew25.php
- Make SSS custom reports visible (again)
- Default start draw is manual after Eventor import 

13 June 2015 3.2.451.2
- Persist name of entry list import file
- Add custom reports to SSS competition file

30 May 2015 3.2.451.1
- Merge with standard MEOS 3.2.481 (MeOS 3.2)
  For changes see http://www.melin.nu/meos/en/whatsnew25.php

25 February 2015 3.1.406.1
- Merge with standard MEOS 3.1.401 (MeOS 3.1 Update 3)
  For changes see http://www.melin.nu/meos/en/whatsnew25.php
- Correct bug that prevented MEOS-OZ working in server mode.

10 December 2014 3.1.383.5
- Allow user to save details of rental sticks so that rental
flag is set automatically.
- Prevent crash if trying to print rogaining result splits for 
someone who hasn't run.

24 October 2014 3.1.383.4
- Improve quality of SSS upload so that uploaded data can be used
  for RouteGadget.

16 July 2014 3.1.383.3
- Add support for printing labels as competitor finishes or
  afterwards. Configure via splits printing options.

8 May 2014 3.1.383.2
- Australianise the competition report and add reporting on
  competitors/course for simple (1 course per class) events.

5 May 2014 3.1.383.1
- Merge with standard MEOS 3.1.383.1  (the 3.1 release)
  For changes see http://www.melin.nu/meos/en/whatsnew25.php
  
22 March 2014 3.1.361.1
- Merge with standard MEOS 3.1.361.
  For changes see http://www.melin.nu/meos/en/whatsnew25.php

22 February 2014 3.1.316.2 
- Display Rogaining (score) points when download occurs, along 
  with existing result information.

9 February 2014 3.1.316.1 
- Merge with new release of MEOS (3.1.316 - 3.1 Beta)

8 December 2013 (fork of 3.0.311)
- When importing IOF 3 XML entries attempt match on existing entries by name first.
- Club names were not being included in import from XML.

20 October 2013 (fork of 3.0.311)
- Improve formatting of SSS style results so that position numbers aren't truncated.
- Import from XML tries to match existing runner by name if match by external Id fails.
- Treat competitors who have explicit course set the same as those who have course set from class.
- SSS quick-start now uses current date as event date.
- Import from XML now sets class based on imported name if XML doesn't contain CourseId.


14 October 2013 (fork of 3.0.311)
- Implement upload to existing SSS processor.
- Some additional language conversion for multi-stage events.


13 August 2013 (fork of 3.0.311)
- Alter "Remaining Runners" to show who may be missing 
  when running with people being auto-entered as they
  download. Also show when stick used by an existing
  runner was later re-used and hasn't downloaded.

13 July 2013 (fork of 3.0.311)
- Splits shown incorrectly when more than one control with
  a minimum-time (i.e. two road crossings).

26 June 2013 (fork of 3.0.311)
- add missing HTML help for English

19 June 2013 (fork of 3.0.311)
- SSS results report
- Start & finish time converted from UTC in results export
- Replace remaining reference to Swedish Eventor

20 May 2013 (fork of 3.0.311)
- fix export to XML by course
- English header line for OE csv export

May 2013 (fork of 3.0.311)
- New simple drawing mechanism, accessed via Classes|Draw Several Classes
  MEOS-Random Fill - then Draw Automatically.
  This will do a random draw over the range of start times, only using
  times where nobody else is running on same course.

April 2013 (fork of 3.0.311)

Australian Customisations:
- New list, results by course, hard-coded
- Enhance line course splits printout
- Import of Or format start lists
- Automatic set-up of Sydney Summer Series event
- Default to Australian Eventor
- Include course in by-minute start list
- Some changes to start draw to compress starts
- Format score (rogaine) splits in a more meaningful way
- Supply Australian competitor/club database not Swedish one
- Shorten long club names provided by Eventor to XX.Y format


Bug Fixes (to be sent to MEOS owners):
- oRunner::getLegPlace() doesn't work.

Bug Fixes (sent to MEOS owner):
- oRunner::doAdjustTimes() - splits shown incorrectly 
  when more than one control with a minimum-time.
- New time-zone adjustment logic failed if the supplied
  timestamp was in hh:mm:ss format.