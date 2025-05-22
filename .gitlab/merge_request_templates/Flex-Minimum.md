# JIRA Ticket(s) Referenced:

FSPRA-XXXX

# Summary:

(Any important information needed for MR review)

# Review Checklist

IMPORTANT: Checking these items only means they have been reviewed.  It does not mean the module is free of related defects.

IMPORTANT: This review checklist is not exhaustive and is not a replacement for quality group code review.

General:
 - [ ] Jira complete: The scope of work described in the Jira(s) is complete.
 - [ ] No unrelated changes: No unrelated changes are made.  Each change must be described in an associated Jira ticket.  A single ticket may describe multiple related issues.
 - [ ] Compatibility: All changes, including changes to module description XML files, must be backward compatible with existing projects.  Any changes that are not backward compatible must have approval from marketing.
 - [ ] Ensure all header files are surrounded by extern "C" guards (See [here](http://wikijs.pbubaap01.rea.renesas.com/en/process/cpp-support)).
 - [ ] Ensure that there are no anonymous structures (See [here](http://wikijs.pbubaap01.rea.renesas.com/en/process/cpp-support)).

XML:
 - [ ] If the XML is updated, verify the changes are covered by the XML testing for this module.
 - [ ] If a new `<config>` block is added to the XML, ensure that the content is surrounded by extern "C" guards (See [here](http://wikijs.pbubaap01.rea.renesas.com/en/process/cpp-support)).

Pipeline:
 - [ ] Full pipeline with all modules has succeeded. This means running a pipeline like the nightly on this branch. Do this by removing your feature branch from pipeline_builds.yml (if it exists), updating the 'develop' entry in pipeline_builds.yml to include your module, and then run a full pipeline by choosing your feature branch and not entering any further variables.
